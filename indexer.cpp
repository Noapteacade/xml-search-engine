#include "indexer.h"
#include "lexer.hpp"
#include <pugixml.hpp>
#include <iostream>
#include <fstream>

std::string enumerate(pugi::xml_node node, int depth = 0) {
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
std::string extract_documentation(const std::string& file_path) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file_path.c_str());
	if (!result) {
		std::cerr << "ERROR: " << result.description() << " at " << result.offset << " bytes" << std::endl;
		return "";
	}
	pugi::xml_node root = doc.document_element();
	return enumerate(root);
}
Document generate_tf(const std::string& content) {
	Document doc;
	Lexer lexer(content);
	std::string token = lexer.next_token();
	while (token != "") {
		token = lexer.next_token();
		if (token == "") continue;
		std::transform(token.begin(), token.end(), token.begin(), ::tolower);
		doc.freq[token]++;
		doc.total_terms++;
	}
	doc.sorted.assign(doc.freq.begin(), doc.freq.end());
	std::sort(doc.sorted.begin(), doc.sorted.end(), [](const auto& a, const auto& b) {
		return a.second > b.second;
		});
	return doc;
}
void dump_index(const TFIndex& index, const Metadata& metadata, const fs::path& path) {
	json j;
	for (const auto& [metadata_k, metadata_v] : metadata) {
		j["!metadata"][metadata_k] = metadata_v;
	}
	for (const auto& [doc_path, doc] : index) {
		json doc_json;
		for (const auto& [term, freq] : doc.freq) {
			doc_json[term] = freq;
		}
		j[doc_path] = doc_json;
	}

	std::ofstream file(path);
	file << j.dump(2);
}
bool load_index(TFIndex& index, const fs::path& path) {
	try {
		std::ifstream file(path);
		if (!file.is_open()) return false;
		json j;
		file >> j;
		std::unordered_map<std::string, std::string> metadata;
		if (j.empty()) return false;

		for (auto& [doc_path, doc_json] : j.items()) {
			std::string full_path = doc_path;
			if (doc_path == "!metadata") {
				for (auto& [key, value] : doc_json.items()) {
					metadata[key] = value.get<std::string>();
				}
				continue;
			};
			Document doc;
			for (const auto& [term, freq] : doc_json.items()) {
				doc.freq[term] = freq.get<uint64_t>();
				doc.total_terms += freq.get<uint64_t>();
			}
			if (auto it = metadata.find("root"); it != metadata.end()) {
				full_path = it->second + "/" + doc_path;
			}
			index[std::move(full_path)] = doc;
		}

		return true;
	}
	catch (const json::parse_error& e) {
		std::cerr << "ERROR: JSON parse error: " << e.what() << std::endl;
		return false;
	}
	catch (const std::exception& e) {
		std::cerr << "ERROR: Unexpected error: " << e.what() << std::endl;
		return false;
	}
}
