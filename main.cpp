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

#include "commandline.hpp"
#include "document.h"
#include "aliases.h"
#include "lexer.hpp"
#include "indexer.h"
#define TODO std::cerr << "TODO: Not implemented yet" \
	<< " at " << __FILE__ << ":" << __LINE__ << std::endl; \
	exit(1);


//std::string extract_documentation(const std::string& file_path);
//static Document generate_tf(const std::string& content);
//static void dump_index(const TFIndex& index, const Metadata& metadata, const fs::path& path);
//static bool load_index(TFIndex& index, const fs::path& path);
static float tf(const std::string& term, const Document& doc);
static float idf(const std::string& term, const TFIndex& index);


class SubcommandHandler {
public:
	static int index(int argc, char* argv[]) {
		commandline::Arguments args(argc, argv);
		args.flag_position_argument(1, "directory");
		args.parse();
		fs::path path = fs::absolute(args["directory"].get_value());
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
	static int search(int argc, char* argv[]) {
		if (argc < 4) {
			std::cerr << "ERROR: Path argument and indexfile argument is required for subcommand `search`";
			return 1;
		}
		commandline::Arguments args(argc, argv);
		args.flag_position_argument(1, "keyword");
		args.flag_position_argument(2, "indexfile");
		args.parse();
		std::string keyword = args["keyword"].get_value();
		std::string file = args["indexfile"].get_value();
		std::string topN = args["top"].get_value();
		fs::path indexfile = fs::absolute(file);
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
		std::cout << "Best match: " << scores[0].first << std::endl;
		std::cout << "Other matches: \n";
		size_t top_n = (topN == "" ? scores.size() : stoi(topN));
		for (size_t i = 1; i < std::min(scores.size(), top_n); ++i) {
			std::cout << "  " << scores[i].first << " (score: " << scores[i].second << ")\n";
		}
		return 0;
	}
	static void badcommand(int argc, char* argv[]) {
		std::cerr << "ERROR: Unknown subcommand: " << argv[1]
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
	// argument syntax for subcommand `index`: index [path] => generate index.json
	// argument syntax for subcommand `search`: todo
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <subcommand> [args...]" << std::endl;
		return 1;
	}
	std::string subcommand = argv[1];
	try {
		if (subcommand == "index") SubcommandHandler::index(argc, argv);
		else if (subcommand == "search") SubcommandHandler::search(argc, argv);
		else SubcommandHandler::badcommand(argc, argv);
	}
	catch (const std::exception& e) {
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	return 0;
}