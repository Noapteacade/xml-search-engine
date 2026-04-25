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
#include <CLI\CLI.hpp>

#include "document.h"
#include "aliases.h"
#include "lexer.hpp"
#include "indexer.h"
#define TODO std::cerr << "TODO: Not implemented yet" \
	<< " at " << __FILE__ << ":" << __LINE__ << std::endl; \
	exit(1);

static float tf(const std::string& term, const Document& doc);
static float idf(const std::string& term, const TFIndex& index);


class SubcommandHandler {
public:
	static int index(const std::string& _path) {
		fs::path path = fs::absolute(_path);
		TFIndex index;
		for (const auto& entry : fs::recursive_directory_iterator(path)) {
			if (entry.is_regular_file() &&
				(
					entry.path().extension() == ".xml" ||
					entry.path().extension() == ".xhtml"
					)
				) {
				std::string full_path = entry.path().string();
				std::cout << "Indexing " << full_path << std::endl;
				std::string doc_path = fs::relative(entry.path(), path).string();
				std::string documentation = extract_documentation(full_path);
				Document doc = generate_tf(documentation);
				index[doc_path] = std::move(doc);
			}
		}
		if (!fs::exists("index-files")) fs::create_directories("index-files");
		Metadata metadata;
		metadata["root"] = path.generic_string();
		dump_index(index, metadata, "index-files\\index_" + path.filename().string() + ".json");
		return 0;
	}
	static int search(const std::string& keyword, const std::string& _index_file, size_t top_n) {
		fs::path indexfile = fs::absolute(_index_file);
		std::cout << "searching " << keyword << " in " << indexfile.generic_string() << std::endl;
		Lexer lexer(keyword);
		std::string token = " ";
		TFIndex index;
		if (!load_index(index, indexfile)) {
			std::cerr << "ERROR: load index file failed: " << indexfile.generic_string() << std::endl;
			return 1;
		}
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
				std::cout << "  " << fs::relative(scores[i].first).generic_string() << " (score: " << scores[i].second << ")\n";
			}
		}
		return 0;
	}
	static int contains(const std::string& _indexfile) {
		fs::path indexfile = fs::absolute(_indexfile);
		TFIndex index;
		if (!load_index(index, indexfile)) {
			std::cerr << "ERROR: load index file failed: " << indexfile.generic_string() << std::endl;
			return 1;
		}
		uint64_t files = 0;
		for (auto it = index.begin(); it != index.end(); it++) {
			files++;
		}
		std::cout << "Index " << indexfile.generic_string() << " Contains " << files << " files";
		return 0;
	}
	static void badcommand(const std::string& subcommand) {
		std::cerr << "ERROR: Unknown subcommand: " << subcommand
			<< "\n     Vaild subcommand: `index` `search`" << std::endl;
		exit(1);
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
	search_cmd->add_option("indexfile", index_file, "Index file path")->required();
	search_cmd->add_option("--top", top_n, "Number of results")->default_val(10);

	auto* contains_cmd = app.add_subcommand("contains", "Count index file indexed files");
	contains_cmd->add_option("indexfile", index_file, "Index file path")->required();

	CLI11_PARSE(app, argc, argv);

	if (index_cmd->parsed()) {
		SubcommandHandler::index(index_path);
	}
	else if (search_cmd->parsed()) {
		SubcommandHandler::search(keyword, index_file, top_n);
	}
	else if (contains_cmd->parsed()) {
		SubcommandHandler::contains(index_file);
	}
	return 0;
}