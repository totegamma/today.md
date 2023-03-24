#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include <ranges>
#include <regex>
#include <map>

#include <CLI/CLI.hpp>

std::string defaultTemplate = 
    #include "defaultTemplate.txt"
;
std::string pre_commit_file = 
    #include "pre-commit.txt"
;

struct config_t {
    std::string editor;
    std::string path;
    std::string memo;
    std::vector<std::string> projects;
    std::string archivePathPattern;
    int projectID;
    int newID;
    bool configured;
    CLI::App* sub_init;
    CLI::App* sub_projects;
    CLI::App* sub_switch;
    CLI::App* sub_memo;
    CLI::App* sub_currprj;
    CLI::Option* archivePathPattern_option;
    CLI::Option* projectID_option;
    CLI::Option* conf_option;
    CLI::Option* projects_option;
    CLI::Option* memo_option;
};

void writeoutConfig(CLI::App& app) {
    std::ofstream out(std::string(getenv("HOME")) + "/.today.conf");
    out << app.config_to_str(true, true);
    out.close();
}

std::string getTimeString() {
    time_t t = time(nullptr);
    const tm* localTime = localtime(&t);
    std::stringstream s;
    s << std::setw(2) << std::setfill('0') << localTime->tm_hour;
    s << std::setw(2) << std::setfill('0') << localTime->tm_min;
    return s.str();
}

std::string getDateString(std::string format) {
    const std::time_t t = std::time(nullptr);
    const std::tm* localtime = std::localtime(&t);
    std::stringstream s;
    s << std::put_time(localtime, format.c_str());
    return s.str();
}

std::string getFileDate(std::string path, std::string format) {
    if (!std::filesystem::exists(path)) return "";

    std::filesystem::file_time_type tp = std::filesystem::last_write_time(path);
    std::chrono::sys_time st = std::chrono::file_clock::to_sys(tp);
    std::time_t t = std::chrono::system_clock::to_time_t(st);
    const std::tm* localtime = std::localtime(&t);

    std::stringstream s;
    s << std::put_time(localtime, format.c_str());
    return s.str();
}

void rotate(config_t& conf) {
    std::string path = conf.projects[conf.projectID] + getFileDate(conf.projects[conf.projectID] + "/today.md", conf.archivePathPattern);

    if (!std::filesystem::is_directory(path)) {
        std::filesystem::create_directories(path);
    }

    if (std::filesystem::exists(conf.projects[conf.projectID] + "today.md")) {
        std::filesystem::rename(conf.projects[conf.projectID] + "today.md", path + "today.md");
    }
}

void registerOptions(CLI::App& app, config_t& conf) {
    app.set_config("--config", ".today.conf", "Read an ini file")
        -> transform(CLI::FileOnDefaultPath(getenv("HOME"), false));

    app.add_option("--editor", conf.editor, "editor")
        -> default_val("vim");
    conf.archivePathPattern_option = app.add_option("--archivePathPattern", conf.archivePathPattern, "archive path pattern")
        -> default_val("%Y/%m/%d/");
    conf.projectID_option = app.add_option("--id", conf.projectID, "project ID")
        -> default_val("0");
    conf.projects_option = app.add_option("--projects", conf.projects, "project list");

    conf.conf_option = app.add_option("--configured", conf.configured, "hidden option")
        -> group("");

    conf.sub_init = app.add_subcommand("init", "initialize working directory");
    conf.sub_init->add_option("path", conf.path, "deploy path")
        -> default_val(".")
        -> configurable(false);

    conf.sub_projects = app.add_subcommand("projects", "show projects");

    conf.sub_switch = app.add_subcommand("switch", "switch project");
    conf.sub_switch->add_option("id", conf.newID, "project id to switch to")
        -> configurable(false)
        -> required(true);

    conf.sub_memo = app.add_subcommand("memo", "write memo");
    conf.memo_option = conf.sub_memo->add_option("filename", conf.memo, "memo file name")
        -> configurable(false);

    conf.sub_currprj = app.add_subcommand("currprj", "show current project directory");
}

void splitIntoSections(std::string input, std::map<std::string, std::string>& sections) {
    std::regex re(R"(^# (.*))");
    std::smatch m;

    std::string sectionName = "_head";

    for (std::string elem : input | std::views::split(std::views::single('\n')) 
                                  | std::views::transform([](auto a) {
                                        auto b = a | std::views::common;
                                        return std::string{b.begin(), b.end()};
                                    })) {
        if (std::regex_match(elem, m, re)) {
            sectionName = m[1].str();
            sections[sectionName] = "";
        } else {
            sections[sectionName] += elem + "\n";
        }
    }
}

namespace commands {
    int init(CLI::App& app, config_t& conf) {
        std::string path = std::filesystem::absolute(conf.path);
        if (path[path.size()-1] == '.')
            path = path.substr(0, path.size() -1);
        if (std::find(conf.projects.begin(), conf.projects.end(), path) == conf.projects.end()) {

            if (!conf.configured) conf.conf_option->add_result("true");
            conf.projects_option->add_result(path);

            system(std::string("git init " + path).c_str());

            std::ofstream out_template(path + "/.template.md");
            out_template << defaultTemplate;
            out_template.close();

            std::filesystem::create_directories(path + "/.git/hooks");
            std::ofstream out_precommit(path + "/.git/hooks/pre-commit");
            out_precommit << pre_commit_file;
            out_precommit.close();
            std::filesystem::permissions(path + "/.git/hooks/pre-commit", std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);

            writeoutConfig(app);

        } else {
            std::cout << "project already registerd!" << std::endl;
            return -1;
        }
        return 0;
    }

    int memo(CLI::App& app, config_t& conf) {
        std::string path = conf.projects[conf.projectID] + getDateString(conf.archivePathPattern);

        if (!std::filesystem::is_directory(path)) {
            std::filesystem::create_directories(path);
        }

        system(std::string(conf.editor + " " + path + (conf.memo_option->empty() ? getTimeString() : conf.memo) + ".md").c_str());

        return 0;
    }

    int projects(CLI::App& app, config_t& conf) {
        auto output = std::views::iota(0, (int)conf.projects.size())
                    | std::views::transform([&](int i){
                            if (i == conf.projectID) {
                                return "\e[33m[" + std::to_string(i) + "] " + conf.projects[i] + "\e[0m";
                            } else {
                                return "[" + std::to_string(i) + "] " + conf.projects[i];
                            }
                        })
                    | std::views::common;
        std::copy(output.begin(), output.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
        return 0;
    }

    int switchto(CLI::App& app, config_t& conf) {
        if (conf.newID < 0 || conf.projects.size() <= conf.newID) {
            std::cout << "project ID out of range" << std::endl;
            return -1;
        }

        conf.projectID_option->clear();
        conf.projectID_option->add_result(std::to_string(conf.newID));

        writeoutConfig(app);
        return 0;
    }

    int currprj(CLI::App& app, config_t& conf) {
        std::cout << conf.projects[conf.projectID] << std::endl;
        return 0;
    }

    int today(CLI::App& app, config_t& conf) {
        std::map<std::string, std::string> sections;
        std::string projectDir = conf.projects[conf.projectID];

        if (getFileDate(projectDir + "/today.md", conf.archivePathPattern) != getDateString(conf.archivePathPattern)) {
            std::ifstream t(projectDir + "/today.md");
            std::stringstream buffer;
            buffer << t.rdbuf();
            splitIntoSections(buffer.str(), sections);
            rotate(conf);
        }

        if (!std::filesystem::exists(projectDir + "/today.md")) {
            if (std::filesystem::exists(projectDir + "/.template.md")) {

                std::ifstream t(projectDir + "/.template.md");
                std::stringstream template_buf;
                template_buf << t.rdbuf();
                std::string template_text = template_buf.str();
                std::string newfile;

                std::regex shortcode = std::regex(R"(\{\{(.*)\}\})");
                std::string::const_iterator cb = template_text.cbegin();
                std::string::const_iterator cd = template_text.cend();

                for (std::smatch match; std::regex_search(cb, cd, match, shortcode); cb = match[0].second) {
                    newfile += match.prefix();
                    newfile += sections[match[1].str()];
                }
                newfile.append(cb, cd);

                std::ofstream out(projectDir + "/today.md");
                out << newfile;
                out.close();
            }
        }
        system(std::string(conf.editor + " " + projectDir + "/today.md").c_str());
        return 0;
    }
}

int main(int argc, char** argv) {
    CLI::App app{"today.md: document based todo management tool"};
    config_t conf;

    registerOptions(app, conf);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (app.got_subcommand(conf.sub_init))
        return commands::init(app, conf);

    if (!conf.configured) {
        std::cout << "There are no configuration file!" << std::endl;
        std::cout << "You can create it by hand, or just do 'today init' to Setup automatically." << std::endl;
        return -1;
    }

    if (app.got_subcommand(conf.sub_projects)) {
        return commands::projects(app, conf);
    }
    if (app.got_subcommand(conf.sub_switch)) {
        return commands::switchto(app, conf);
    }
    if (app.got_subcommand(conf.sub_memo)) {
        return commands::memo(app, conf);
    }
    if (app.got_subcommand(conf.sub_currprj)) {
        return commands::currprj(app, conf);
    }

    // default
    return commands::today(app, conf);

    // never reach
    return 0;
}
