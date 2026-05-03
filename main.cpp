#include <pugixml.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <stdint.h>
#include <cctype>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <CLI\CLI.hpp>

#include "document.h"
#include "aliases.h"
#include "lexer.hpp"
#include "indexer.h"
#define TODO std::cerr << "TODO: Not implemented yet" \
	<< " at " << __FILE__ << ":" << __LINE__ << std::endl; \
	exit(1);

#define ERROR(reason) do { \
	std::cerr << "ERROR: " << (reason) \
	<< " at " << __FILE__ << ":" << __LINE__ << std::endl; \
	exit(1); \
	} while(0)

#define ERROR_IF(cond, reason) if (cond) ERROR(reason);
static float tf(const std::string& term, const Document& doc);
static float idf(const std::string& term, const TFIndex& index);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
	os << "[";
	for (size_t i = 0; i < vec.size(); ++i) {
		if (i != 0) os << ", ";
		os << vec[i];
	}
	os << "]";
	return os;
}
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& uset) {
	os << "[";
	for (auto it = uset.begin(); it != uset.end(); ++it) {
		if (it != uset.begin()) os << ", ";
		os << *it;
	}
	os << "]";
	return os;
}
template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m) {
	os << "[";
	for (auto it = m.begin(); it != m.end(); ++it) {
		if (it != m.begin()) os << ", ";
		os << "{" << it->first << ": " << it->second << "}";
	}
	os << "]";
	return os;
}

class SubcommandHandler {
private:
	inline static fs::path inverted = "index-files/inverted_index.json";
	static inline std::string index_path(const std::string& str) {
		return "index-files/index_" + str + ".json";
	}
	static inline std::unordered_set<std::string> searches(
		const std::unordered_map<std::string, std::vector<std::string>>& map
	) {
		// The intersection of the extracted values (i.e.
		// map={{"!":{"index a", "index b"}, {"@":{"index a"}}}}, result={"index a"})

		if (map.empty()) {
			return {};
		}

		// Start with the first vector as the initial set
		auto it = map.begin();
		std::unordered_set<std::string> result(it->second.begin(), it->second.end());

		// Intersect with each subsequent vector
		for (++it; it != map.end(); ++it) {
			const auto& current_vec = it->second;
			std::unordered_set<std::string> temp;

			// Only keep elements that exist in both result and current_vec
			for (const auto& str : current_vec) {
				if (result.find(str) != result.end()) {
					temp.insert(str);
				}
			}

			result = std::move(temp);

			// Early exit if intersection becomes empty
			if (result.empty()) {
				break;
			}
		}

		return result;
	}
public:
	static int index(const std::string& _path) {
		fs::path path = fs::absolute(_path);
		if (!fs::exists("index-files")) fs::create_directories("index-files");
		std::unordered_map<std::string, TFIndex> groups;
		Metadata metadata;
		metadata["root"] = path.generic_string();
		for (const auto& entry : fs::recursive_directory_iterator(path)) {
			if (!entry.is_regular_file() ||
				!(
					entry.path().extension() == ".xhtml" ||
					entry.path().extension() == ".xml"
					)
				) {
				std::cout << "WARN: cannot parse path: " << entry.path().generic_string() << ", skipped" << std::endl;
				continue;
			}
			// 提取顶层子目录（相对于 path 的第一级）
			auto relative = fs::relative(entry.path(), path);
			std::string top_dir = relative.begin()->filename().string();

			std::string full_path = entry.path().string();
			std::cout << "Indexing " << full_path << std::endl;
			std::string doc_path = relative.string();
			groups[top_dir][doc_path] = generate_tf(extract_documentation(full_path));
		}

		// inverted index k-v syntax:
		// "term": ["index file a", "index file b"]
		// generate inverted index
		std::unordered_map<std::string, std::unordered_set<std::string>> inverted_index;
		for (const auto& [dir, idx] : groups) {
			for (auto& [doc_path, doc] : idx) {
				for (auto& [term, _] : doc.freq) {
					inverted_index[term].insert(index_path(dir));
				}
			}
		}
		json inverted_index_json;
		for (const auto& [term, files] : inverted_index) {
			inverted_index_json[term] = files;
		}
		std::ofstream inverted_index_file(inverted);
		ERROR_IF(!inverted_index_file.is_open(),
			"cannot create inverted index file: " + inverted.generic_string());
		inverted_index_file << inverted_index_json.dump(2);

		for (auto& [dir, idx] : groups) {
			dump_index(idx, metadata, index_path(dir));
		}
		return 0;
	}
	static int search_implicit(const std::string& keyword, size_t top_n) {
		// --- get search list from inverted index --
		// -- load inverted index to json --
		json inverted_j;
		std::ifstream inverted_index_file(inverted);
		ERROR_IF(!inverted_index_file.is_open(),
			"cannot open inverted index file: " + inverted.generic_string());
		inverted_index_file >> inverted_j;
		// -- filter for the keywords that match --
		// - tokenize -
		Lexer lexer(keyword);
		std::unordered_set<std::string> tokens;
		std::string token = " ";
		while (token != "") {
			token = lexer.next_token();
			std::transform(token.begin(), token.end(), token.begin(), ::tolower);
			if (token != "") tokens.insert(token);
		}
		// - filter inverted_j by tokens -
		std::unordered_map<std::string, std::vector<std::string>> map;
		for (const auto& tok : tokens) {
			if (auto it = inverted_j.find(tok);
				it != inverted_j.end()) {
				map[tok] = it.value();
			}
			else {
				std::cout << "WARN: token " << tok << " not found in inverted index, skipped" << std::endl;
			}
		}
		const auto search_list = searches(map);
		// --- search in search list ---
		std::cout << "searching " << keyword << " in " << search_list << std::endl;
		TFIndex index;
		std::unordered_map<std::string, float> doc_score;
		std::vector<std::pair<std::string, float>> scores;
		for (const auto& file : search_list) {
			doc_score.clear();
			fs::path indexfile = fs::absolute(file);
			ERROR_IF(!load_index(index, indexfile), "load index file failed: " + indexfile.generic_string());
			for (const auto& token : tokens) {
				for (const auto& [path, table] : index) {
					const auto score_tf = tf(token, table);
					const auto score_idf = idf(token, index);
					const auto score = score_tf * score_idf;
					if (score > 0) doc_score[path] += score;
				}
			}
			scores.insert(scores.end(), doc_score.begin(), doc_score.end());
		}
		std::sort(scores.begin(), scores.end(),
			[](const std::pair<std::string, float> a,
				const std::pair<std::string, float> b) {
					return a.second > b.second;
			});

		if (scores.empty()) {
			std::cout << "No results found for keyword: " << keyword << std::endl;
			return 0;
		}
		std::cout << "Best match: " << fs::relative(scores[0].first).generic_string() << std::endl;
		if (scores.size() > 1) {
			std::cout << "Other matches: \n";
			for (size_t i = 1; i < std::min(scores.size(), top_n); ++i) {
				std::cout << "  "
					<< std::left << std::setw(60)
					<< fs::relative(scores[i].first).generic_string()
					<< " (score: " << std::fixed
					<< std::setprecision(4)
					<< scores[i].second
					<< ")\n";
			}
		}
	}
	static int search(const std::string& keyword, const std::string& _index_file, size_t top_n) {
		fs::path indexfile;
		if (_index_file.empty()) return search_implicit(keyword, top_n);
		else indexfile = fs::absolute(_index_file);
		std::cout << "searching " << keyword << " in " << indexfile.generic_string() << std::endl;
		Lexer lexer(keyword);
		std::string token = " ";
		TFIndex index;
		ERROR_IF(!load_index(index, indexfile), "load index file failed: " + indexfile.generic_string())
			std::unordered_map<std::string, float> doc_score;
		while (token != "") {
			token = lexer.next_token();
			std::transform(token.begin(), token.end(), token.begin(), ::tolower);
			for (auto& [path, table] : index)
			{
				const auto score_tf = tf(token, table);
				const auto score_idf = idf(token, index);
				const auto score = score_tf * score_idf;
				if (score > 0) doc_score[path] += score;
			}
		}
		std::vector<std::pair<std::string, float>> scores(doc_score.begin(), doc_score.end());
		std::sort(scores.begin(), scores.end(),
			[](const std::pair<std::string, float> a,
				const std::pair<std::string, float> b) {
					return a.second > b.second;
			});
		if (scores.empty()) {
			std::cout << "No results found for keyword: " << keyword << std::endl;
			return 0;
		}
		std::cout << "Best match: " << fs::relative(scores[0].first).generic_string() << std::endl;
		if (scores.size() > 1) {
			std::cout << "Other matches: \n";
			for (size_t i = 1; i < std::min(scores.size(), top_n); ++i) {
				std::cout << "  "
					<< std::left << std::setw(60)
					<< fs::relative(scores[i].first).generic_string()
					<< " (score: " << std::fixed
					<< std::setprecision(4)
					<< scores[i].second
					<< ")\n";
			}
		}
		return 0;
	}
	static int contains(const std::string& _indexfile) {
		fs::path indexfile = fs::absolute(_indexfile);
		TFIndex index;
		ERROR_IF(!load_index(index, indexfile), "load index file failed: " + indexfile.generic_string())
			uint64_t files = 0;
		for (auto it = index.begin(); it != index.end(); it++) {
			files++;
		}
		std::cout << "Index " << indexfile.generic_string() << " Contains " << files << " files";
		return 0;
	}
	static void badcommand(const std::string& subcommand) {
		ERROR("Unknown subcommand: " + subcommand + "\n     Vaild subcommand: `index` `search` `contains`");
	}
};

static float tf(const std::string& term, const Document& doc) {
	auto it = doc.freq.find(term);
	if (it == doc.freq.end()) return 0.0f;
	return static_cast<float>(it->second) / doc.total_terms;
}
static float idf(const std::string& term, const TFIndex& index) {
	const uint64_t N = index.size();
	const uint64_t M = std::count_if(index.begin(), index.end(),
		[&term](const auto& pair) {
			return pair.second.freq.find(term) != pair.second.freq.end();
		});
	return static_cast<float>(std::log(1.0 * N / (1 + M)));
}
int main(int argc, char* argv[]) {
	CLI::App app{ "LocalSearchEngine" };
	auto* index_cmd = app.add_subcommand("index", "Build index from directory");
	std::string index_path;
	index_cmd->add_option("directory", index_path, "Directory to index")->required();

	auto* search_cmd = app.add_subcommand("search", "Search indexed documents");
	std::string keyword;
	std::string index_file;
	int top_n = 10;
	search_cmd->add_option("keyword", keyword, "Search keyword")->required();
	search_cmd->add_option("indexfile", index_file, "Index file path");
	search_cmd->add_option("--top", top_n, "Number of results")->default_val(10);

	auto* contains_cmd = app.add_subcommand("contains", "Count index file indexed files");
	contains_cmd->add_option("indexfile", index_file, "Index file path")->required();

	CLI11_PARSE(app, argc, argv);

	try {
		if (index_cmd->parsed()) {
			SubcommandHandler::index(index_path);
		}
		else if (search_cmd->parsed()) {
			SubcommandHandler::search(keyword, index_file, top_n);
		}
		else if (contains_cmd->parsed()) {
			SubcommandHandler::contains(index_file);
		}
	}
	catch (const std::exception& e) {
		ERROR(e.what());
	}
	return 0;
}