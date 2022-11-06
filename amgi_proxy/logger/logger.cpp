#include "logger.h"

#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <string>

namespace logging {
    logger::logger() : output_{output::console}, level_{level::info} {}

    logger& logging::logger::get() {
        if (instance_)
            return *instance_;
        else {
            create();
            return *instance_;
        }
    }

    void logger::create() {
        std::locale::global(std::locale(""));
        static logger the_instance;
        instance_ = &the_instance;
    }

    void logger::initialize(const std::string& file_path, logger::output log_output, logger::level log_level) {
        get().initialize_impl(file_path, log_output, log_level);
    }

    void logger::initialize_impl(const std::string& file_path, output log_output, level log_level) {
        if (log_output == output::file || log_output == output::file_and_console)
            file_ = std::ofstream(file_path);
        output_ = log_output;
        level_ = log_level;
    }

    std::string logger::make_log_record_prefix(const level log_level) {
        auto time = timestamp();
        auto wrapped_level = "[" + log_level_to_string(log_level) + "]\t";
        return time + ' ' + wrapped_level;
    }

    std::string logger::log_level_to_string(level log_level) {
        const static std::unordered_map<level, std::string> level_string_map {
            {level::debug,   "debug"},
            {level::trace,   "trace"},
            {level::info,    "info"},
            {level::warning, "warn"},
            {level::error,   "error"},
            {level::fatal,   "fatal"},
        };

        if (const auto it = level_string_map.find(log_level); it != level_string_map.end())
            return it->second;

        return {};
    }

    std::string logging::logger::timestamp() {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss.imbue(std::locale());
        ss << std::put_time(localtime(&tt), "%F %T");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return ss.str();
    }

    void logger::log(const std::string& message, logger::level log_level) {
        if (log_level < level_ || message.empty() || output_ == none)
            return;

        auto pre_message = make_log_record_prefix(log_level);
        auto total_message = pre_message + message;
        if (total_message[total_message.size() - 1] != '\n')
            total_message.push_back('\n');

        auto& out_stream = (log_level <= level::info) ? std::cout : std::cerr;
        if (output_ == console || output_ == file_and_console) {
            out_stream << total_message;
            //out_stream.flush();
        }

        if (file_.is_open() && (output_ & file)) {
            file_ << total_message;
            //file_.flush();
        }
    }

    logger::~logger() {
        instance_ = nullptr;
    }
}