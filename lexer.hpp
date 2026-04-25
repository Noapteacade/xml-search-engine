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