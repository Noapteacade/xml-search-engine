#pragma once
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <optional>
namespace commandline
{
	class commandline_error : public std::runtime_error {
	public:
		commandline_error(const std::string& message) : std::runtime_error(message) {}
	};
	class Argument {
	private:
		std::string value;
	public:
		std::string& get_value() {
			return value;
		}
	};
	class Arguments {
	private:
		std::unordered_map<std::string, Argument> args;
		std::vector<std::pair<int, std::string>> position_index;
		int argc;
		std::vector<std::string> argv;
	public:
		Arguments() {}
		Arguments(int argc, char* argv[]) {
			this->argc = argc;
			this->argv = std::vector<std::string>(argv, argv + argc);
			// remove this->argv's first argument, skip executable name
			this->argv.erase(this->argv.begin());
		};
		Argument& operator[](const std::string& name) {
			return args[name];
		}
		void flag_position_argument(int idx, std::string name) {
			position_index.push_back({ idx, name });
		}
		void parse() {
			for (int i = 0; i < argc; i++) {
				std::string argument(argv[i]);
				if (argument.rfind("--") == 0) {
					std::string argument_name = argument.substr(2);
					if (i + 1 < argc) {
						args[argument_name].get_value() = argv[i + 1];
						i++;
					}
					else {
						throw commandline_error("Argument " + argument_name + " expects a value");
					}
				}
				else {
					if (
						auto it = std::find_if(position_index.begin(), position_index.end(),
							[&i](const auto& p) {
								return p.first == i;
							});
							it != position_index.end()) {
						args[(*it).second].get_value() = argument;
					}
				}
			}
		}
		std::optional<Argument> get(std::string name) {
			if (auto it = args.find(name);
				it != args.end()) {
				return it->second;
			}
			else return std::nullopt;
		}
	};
} // namespace commandline