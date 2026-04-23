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
#include <fstream>
#include <optional>

#define TODO std::cerr << "TODO: Not implemented yet" \
	<< " at " << __FILE__ << ":" << __LINE__ << std::endl; \
	exit(1);

namespace fs = std::filesystem;
using json = nlohmann::json;
using TF = std::unordered_map<std::string, uint64_t>;
using TFSorted = std::vector<std::pair<std::string, uint64_t>>;
using TFIndex = std::unordered_map<std::string, TFSorted>;

class Lexer {
	std::string_view content;
	void trim_left() {
		while (!content.empty() && std::isspace(content.front())) {
			content.remove_prefix(1);
		}
	}
	std::string chop_left(size_t n) {
		std::string token(content.substr(0, n));
		content.remove_prefix(n);
		return token;
	}
	template<typename Predicate>
	std::string chop_while(Predicate pred, bool include_last = false) {
		size_t n = 0;
		while (n < content.size() && pred(content[n])) {
			n++;
		}
		if (include_last && n < content.size()) n++;
		return chop_left(n);
	}
public:
	Lexer(const std::string& str) : content(str) {}

	std::string next_token() {
		trim_left();

		if (content.empty()) return "";
		if (!isascii(content.front())) {
			chop_left(1);
			return next_token();
		}
		if (content.substr(0, 2) == "{$") {
			return chop_while([](char c) { return c != '}'; }, true);
		}

		if (std::isdigit(content.front())) {
			return chop_while([](char c) { return std::isdigit(c) || c == '.'; });
		}

		if (std::isalpha(content.front())) {
			return chop_while([](char c) { return std::isalnum(c) || c == '_'; });
		}
		return chop_left(1);
		// TODO;
	}
};
static std::unordered_map<std::string, uint64_t> index(const std::string& str) {
	TODO;
}
static std::string enumerate(pugi::xml_node node, int depth = 0) {
	std::string documentation = "";
	for (pugi::xml_node child : node.children()) {
		if (child.type() == pugi::node_pcdata) {
			std::string text = child.value();
			if (!text.empty()) {
				documentation += text;
			}
		}
		std::string child_text = enumerate(child, depth + 1);
		if (!child_text.empty()) {
			if (!documentation.empty() && documentation.back() != '\n') {
				documentation += ' ';
			}
			documentation += child_text;
		}
	}
	return documentation;
}
static std::string extract_documentation(const std::string& file_path) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file_path.c_str());
	if (!result) {
		std::cerr << "ERROR: " << result.description() << " at " << result.offset << " bytes" << std::endl;
		return "";
	}
	pugi::xml_node root = doc.document_element();
	return enumerate(root);
}
static TFSorted generate_tf(const std::string& content) {
	TF term_freq;
	Lexer lexer(content);
	std::string token = lexer.next_token();
	while (token != "") {
		token = lexer.next_token();
		std::transform(token.begin(), token.end(), token.begin(), ::tolower);
		term_freq[token]++;
	}
	auto sorted = std::vector<std::pair<std::string, uint64_t>>(term_freq.begin(), term_freq.end());
	std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
		return a.second > b.second;
		});
	return sorted;
}
static void dump_index(const TFIndex& index, const std::string& filename) {
	json j;

	for (const auto& [doc_path, tf_sorted] : index) {
		json doc_json;
		for (const auto& [term, freq] : tf_sorted) {
			doc_json[term] = freq;
		}
		j[doc_path] = doc_json;
	}

	std::ofstream file(filename);
	file << j.dump(4);
}
static std::optional<TFIndex> load_index(const std::string& filename) {
	try {
		TFIndex index;
		std::ifstream file(filename);
		if (!file.is_open()) return std::nullopt;
		json j;
		file >> j;

		if (j.empty()) return std::nullopt;

		for (const auto& [doc_path, doc_json] : j.items()) {
			TFSorted tf_sorted;
			for (const auto& [term, freq] : doc_json.items()) {
				tf_sorted.emplace_back(term, freq.get<uint64_t>());
			}
			index[doc_path] = std::move(tf_sorted);
		}

		return index;
	}
	catch (const json::parse_error& e) {
		std::cerr << "ERROR: JSON parse error: " << e.what() << std::endl;
		return std::nullopt;
	}
	catch (const std::exception& e) {
		std::cerr << "ERROR: Unexpected error: " << e.what() << std::endl;
		return std::nullopt;
	}
}
int main(int argc, char* argv[]) {
	// argument syntax for subcommand `index`: index [path] => generate index.json
	// argument syntax for subcommand `search`: todo
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <subcommand> [args...]" << std::endl;
		return 1;
	}
	std::string subcommand = argv[1];
	if (subcommand == "index") {
		if (argc < 3) {
			std::cerr << "ERROR: Path argument is required for index subcommand" << std::endl;
			return 1;
		}
		fs::path path = fs::absolute(argv[2]);
		TFIndex index;
		try {
			for (const auto& entry : fs::recursive_directory_iterator(path)) {
				if (entry.is_regular_file() && 
					(
						entry.path().extension() == ".xml" || 
						entry.path().extension() == ".xhtml"
						)
					) {
					std::string full_path = entry.path().string();
					std::string doc_path = fs::relative(entry.path(), path).string();
					std::string documentation = extract_documentation(full_path);
					TFSorted tf_sorted = generate_tf(documentation);
					index[doc_path] = std::move(tf_sorted);
				}
			}
			if(!fs::exists("index.files")) fs::create_directories("index-files");
			index["!root"] = { std::make_pair(path.generic_string(), 0)};
			dump_index(index, "index-files\\index_" + path.filename().string() + ".json");
		}
		catch (const fs::filesystem_error& e) {
			std::cerr << "ERROR: Filesystem error: " << e.what() << std::endl;
			return 1;
		}
	}
	else if (subcommand == "search") {
		TODO;
	}
	else {
		std::cerr << "ERROR: Unknown subcommand '"
			<< subcommand
			<< "'. Valid subcommands are 'index' and 'search'."
			<< std::endl;
		exit(1);
	}
	return 0;
}