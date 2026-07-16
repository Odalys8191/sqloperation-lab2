#include "../../../include/consumer/computeAPI/officeOperation.h"
#include "../../../include/common/computeType.h"
#include "../../../include/common/grpcClient.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr int kOfficeHostPort = 2341;
constexpr int kContainerXpraPort = 10000;
constexpr int kXpraReadinessAttempts = 150;
constexpr auto kXpraReadinessDelay = std::chrono::milliseconds(100);
constexpr const char *kOfficeImage = "ubuntu_xpra:limited_user";
constexpr const char *kOfficeExecutable = "/opt/libreoffice25.2/program/soffice";
constexpr const char *kAdvertisedXpraHost = "192.168.100.221";

struct CommandResult
{
    int exit_code = -1;
    std::string output;
};

CommandResult runCommand(
    const std::vector<std::string> &arguments,
    const std::vector<std::pair<std::string, std::string>> &environment = {})
{
    CommandResult result;
    if (arguments.empty())
    {
        return result;
    }

    int pipe_fds[2];
    if (pipe(pipe_fds) != 0)
    {
        return result;
    }

    const pid_t pid = fork();
    if (pid < 0)
    {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return result;
    }

    if (pid == 0)
    {
        close(pipe_fds[0]);
        if (dup2(pipe_fds[1], STDOUT_FILENO) < 0 || dup2(pipe_fds[1], STDERR_FILENO) < 0)
        {
            _exit(126);
        }
        close(pipe_fds[1]);

        for (const auto &[name, value] : environment)
        {
            if (setenv(name.c_str(), value.c_str(), 1) != 0)
            {
                _exit(126);
            }
        }

        std::vector<char *> argv;
        argv.reserve(arguments.size() + 1);
        for (const auto &argument : arguments)
        {
            argv.push_back(const_cast<char *>(argument.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv.front(), argv.data());
        _exit(127);
    }

    close(pipe_fds[1]);
    char buffer[4096];
    while (true)
    {
        const ssize_t bytes_read = read(pipe_fds[0], buffer, sizeof(buffer));
        if (bytes_read > 0)
        {
            result.output.append(buffer, static_cast<size_t>(bytes_read));
            continue;
        }
        if (bytes_read < 0 && errno == EINTR)
        {
            continue;
        }
        break;
    }
    close(pipe_fds[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0)
    {
        if (errno != EINTR)
        {
            return result;
        }
    }

    if (WIFEXITED(status))
    {
        result.exit_code = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
        result.exit_code = 128 + WTERMSIG(status);
    }

    return result;
}

std::string trim(const std::string &value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                          return std::isspace(ch) != 0;
                      }).base();
    if (first >= last)
    {
        return {};
    }
    return std::string(first, last);
}

std::string sanitizedDiagnostic(
    std::string diagnostic,
    const std::vector<std::string> &secrets)
{
    for (const auto &secret : secrets)
    {
        if (secret.empty())
        {
            continue;
        }
        size_t position = 0;
        while ((position = diagnostic.find(secret, position)) != std::string::npos)
        {
            diagnostic.replace(position, secret.size(), "[redacted]");
            position += std::strlen("[redacted]");
        }
    }

    constexpr size_t kMaximumDiagnosticLength = 2000;
    if (diagnostic.size() > kMaximumDiagnosticLength)
    {
        diagnostic = diagnostic.substr(diagnostic.size() - kMaximumDiagnosticLength);
    }
    return trim(diagnostic);
}

bool parseContainerExitCode(const std::string &output, int &exit_code)
{
    const std::string value = trim(output);
    if (value.empty())
    {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed < 0 || parsed > 255)
    {
        return false;
    }

    exit_code = static_cast<int>(parsed);
    return true;
}

bool isSafeDcId(const std::string &dc_id)
{
    if (dc_id.empty() || dc_id.size() > 128)
    {
        return false;
    }
    return std::all_of(dc_id.begin(), dc_id.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '-' || ch == '_';
    });
}

bool isAllowedOfficeExtension(const std::string &extension)
{
    static const std::vector<std::string> allowed_extensions = {
        ".docx", ".xlsx", ".pptx", ".doc", ".xls", ".ppt"};
    return std::find(allowed_extensions.begin(), allowed_extensions.end(), extension) != allowed_extensions.end();
}

bool extensionMatchesType(const std::string &extension, int office_type)
{
    static const std::vector<std::string> type_extensions = {
        ".docx", ".xlsx", ".pptx", ".doc", ".xls", ".ppt"};
    return office_type >= 0 &&
           static_cast<size_t>(office_type) < type_extensions.size() &&
           extension == type_extensions[static_cast<size_t>(office_type)];
}

std::string maskIdentifier(const std::string &identifier)
{
    if (identifier.size() <= 12)
    {
        return "[masked]";
    }
    return identifier.substr(0, 8) + "-...-" + identifier.substr(identifier.size() - 4);
}

std::string generateRandomString(size_t length)
{
    static constexpr char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string result;
    result.reserve(length);

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<size_t> distribution(0, sizeof(alphanum) - 2);
    for (size_t i = 0; i < length; ++i)
    {
        result.push_back(alphanum[distribution(generator)]);
    }
    return result;
}

bool isPortAvailable(int port)
{
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        return false;
    }

    int reuse_address = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    const bool available = bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0;
    close(socket_fd);
    return available;
}

bool isPortListening(int port)
{
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    const bool listening = connect(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0;
    close(socket_fd);
    return listening;
}

bool waitForXpraPort(int port)
{
    for (int attempt = 0; attempt < kXpraReadinessAttempts; ++attempt)
    {
        if (isPortListening(port))
        {
            return true;
        }
        std::this_thread::sleep_for(kXpraReadinessDelay);
    }
    return false;
}

bool validateCurrentOfficePolicy(const json &data_capsule_info, std::string &error)
{
    try
    {
        if (!data_capsule_info.contains("OfficeOperation") ||
            !data_capsule_info["OfficeOperation"].is_object())
        {
            error = "OfficeOperation policy is missing";
            return false;
        }

        const json &office_policy = data_capsule_info["OfficeOperation"];
        if (!office_policy.contains("Type") || !office_policy["Type"].is_number_integer() ||
            !office_policy.contains("UsersNumber") || !office_policy["UsersNumber"].is_number_integer() ||
            !office_policy.contains("ABEPolicys") || !office_policy["ABEPolicys"].is_array())
        {
            error = "OfficeOperation policy fields are invalid";
            return false;
        }

        const int office_type = office_policy["Type"].get<int>();
        const int users_number = office_policy["UsersNumber"].get<int>();
        const json &abe_policies = office_policy["ABEPolicys"];
        if (office_type < static_cast<int>(OfficeOperationType::DOCX) ||
            office_type > static_cast<int>(OfficeOperationType::PPT))
        {
            error = "OfficeOperation Type is outside the supported range";
            return false;
        }
        if (users_number <= 0 || abe_policies.empty())
        {
            error = "OfficeOperation policy is exhausted";
            return false;
        }
        if (static_cast<size_t>(users_number) != abe_policies.size())
        {
            error = "UsersNumber does not match ABEPolicys length";
            return false;
        }
        for (const auto &policy : abe_policies)
        {
            if (!policy.is_string() || policy.get<std::string>().empty())
            {
                error = "ABEPolicys contains an invalid policy";
                return false;
            }
        }
    }
    catch (const json::exception &)
    {
        error = "OfficeOperation policy cannot be parsed";
        return false;
    }
    return true;
}

bool buildSuccessorCapsuleInput(
    const json &data_capsule_info,
    json &successor_policy,
    json &successor_attributes,
    std::string &successor_abe_policy,
    int &successor_usable_times,
    bool &policy_exhausted,
    std::string &error)
{
    policy_exhausted = false;
    try
    {
        if (!data_capsule_info.contains("ownerID") || !data_capsule_info["ownerID"].is_string() ||
            data_capsule_info["ownerID"].get<std::string>().empty())
        {
            error = "ownerID is missing";
            return false;
        }

        if (!data_capsule_info.contains("OfficeOperation") ||
            !data_capsule_info["OfficeOperation"].is_object())
        {
            error = "OfficeOperation policy is missing";
            return false;
        }

        json next_office_policy = data_capsule_info["OfficeOperation"];
        if (!next_office_policy.contains("Type") || !next_office_policy["Type"].is_number_integer() ||
            !next_office_policy.contains("UsersNumber") || !next_office_policy["UsersNumber"].is_number_integer() ||
            !next_office_policy.contains("ABEPolicys") || !next_office_policy["ABEPolicys"].is_array())
        {
            error = "OfficeOperation policy fields are invalid";
            return false;
        }

        const int office_type = next_office_policy["Type"].get<int>();
        int users_number = next_office_policy["UsersNumber"].get<int>();
        json abe_policies = next_office_policy["ABEPolicys"];
        if (office_type < static_cast<int>(OfficeOperationType::DOCX) ||
            office_type > static_cast<int>(OfficeOperationType::PPT))
        {
            error = "OfficeOperation Type is outside the supported range";
            return false;
        }
        if (users_number <= 0 || abe_policies.empty() ||
            static_cast<size_t>(users_number) != abe_policies.size())
        {
            error = "OfficeOperation policy is exhausted or inconsistent";
            return false;
        }
        for (const auto &policy : abe_policies)
        {
            if (!policy.is_string() || policy.get<std::string>().empty())
            {
                error = "ABEPolicys contains an invalid policy";
                return false;
            }
        }

        --users_number;
        abe_policies.erase(abe_policies.begin());
        next_office_policy["UsersNumber"] = users_number;
        next_office_policy["ABEPolicys"] = abe_policies;

        if (users_number == 0 && abe_policies.empty())
        {
            policy_exhausted = true;
            return true;
        }
        if (users_number <= 0 || abe_policies.empty() ||
            static_cast<size_t>(users_number) != abe_policies.size())
        {
            error = "OfficeOperation successor policy is inconsistent";
            return false;
        }

        std::vector<int> selected_categories;
        if (data_capsule_info.contains("selectedCategories"))
        {
            if (!data_capsule_info["selectedCategories"].is_array())
            {
                error = "selectedCategories is invalid";
                return false;
            }
            for (const auto &category : data_capsule_info["selectedCategories"])
            {
                if (!category.is_number_integer())
                {
                    error = "selectedCategories contains a non-integer value";
                    return false;
                }
                const int value = category.get<int>();
                if (std::find(selected_categories.begin(), selected_categories.end(), value) == selected_categories.end())
                {
                    selected_categories.push_back(value);
                }
            }
        }

        const int office_compute_type = static_cast<int>(ComputeType::OFFICE_OPERATION);
        if (std::find(selected_categories.begin(), selected_categories.end(), office_compute_type) == selected_categories.end())
        {
            selected_categories.push_back(office_compute_type);
        }

        successor_policy = json::object();
        successor_policy["ComputeType"] = selected_categories;
        successor_policy["OfficeOperation"] = next_office_policy;

        successor_abe_policy = abe_policies.front().get<std::string>();
        successor_attributes = json::object();
        successor_attributes["DataDescription"] = data_capsule_info.value("dataDescription", "");
        successor_attributes["Columns"] = "";
        successor_attributes["AccessStructure"]["1"] = successor_abe_policy;

        if (!data_capsule_info.contains("AccessStructure") ||
            !data_capsule_info["AccessStructure"].is_object() ||
            !data_capsule_info["AccessStructure"].contains("2") ||
            !data_capsule_info["AccessStructure"]["2"].is_string() ||
            data_capsule_info["AccessStructure"]["2"].get<std::string>().empty())
        {
            error = "TEE AccessStructure layer 2 is missing";
            return false;
        }
        successor_attributes["AccessStructure"]["2"] = data_capsule_info["AccessStructure"]["2"];

        successor_usable_times = data_capsule_info.value("usableTimes", 1);
        if (successor_usable_times <= 0)
        {
            successor_usable_times = 1;
        }
    }
    catch (const json::exception &)
    {
        error = "successor capsule input cannot be parsed";
        return false;
    }

    return true;
}

bool produceDataCapsule(
    const json &data_capsule_info,
    const std::string &office_file_path,
    std::string &new_dc_id)
{
    new_dc_id.clear();

    json successor_policy;
    json successor_attributes;
    std::string successor_abe_policy;
    int successor_usable_times = 1;
    bool policy_exhausted = false;
    std::string error;
    if (!buildSuccessorCapsuleInput(
            data_capsule_info,
            successor_policy,
            successor_attributes,
            successor_abe_policy,
            successor_usable_times,
            policy_exhausted,
            error))
    {
        std::cerr << "[OfficeOperation-produceDataCapsule]: " << error << std::endl;
        return false;
    }

    if (policy_exhausted)
    {
        return true;
    }

    std::ifstream input(office_file_path, std::ios::binary);
    if (!input.is_open())
    {
        std::cerr << "[OfficeOperation-produceDataCapsule]: Failed to open the edited Office file." << std::endl;
        return false;
    }
    const std::string office_data(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (input.bad() || office_data.empty())
    {
        std::cerr << "[OfficeOperation-produceDataCapsule]: Failed to read a non-empty edited Office file." << std::endl;
        return false;
    }

    const std::string owner_id = data_capsule_info["ownerID"].get<std::string>();
    ProducerClient producer_client(
        grpc::CreateChannel(PRODUCER_SERVICE_ADDRESS, grpc::InsecureChannelCredentials()));
    new_dc_id = producer_client.generateDataCapsule(
        owner_id,
        successor_policy,
        successor_abe_policy,
        successor_usable_times,
        office_data,
        successor_attributes);

    if (new_dc_id.empty())
    {
        std::cerr << "[OfficeOperation-produceDataCapsule]: Producer did not return a successor capsule ID." << std::endl;
        return false;
    }
    return true;
}

void removeContainer(const std::string &container_name, bool force)
{
    std::vector<std::string> command = {"docker", "rm"};
    if (force)
    {
        command.push_back("--force");
    }
    command.push_back(container_name);
    const CommandResult remove_result = runCommand(command);
    if (remove_result.exit_code != 0)
    {
        std::cerr << "[OfficeOperation-container]: Failed to remove container "
                  << container_name << "." << std::endl;
    }
}

void handleContainerExit(
    const std::string &container_name,
    const std::string &office_file_path,
    const json &data_capsule_info)
{
    const CommandResult wait_result = runCommand({"docker", "wait", container_name});
    if (wait_result.exit_code != 0)
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Failed to monitor container "
                  << container_name << "." << std::endl;
        return;
    }

    int container_exit_code = -1;
    if (!parseContainerExitCode(wait_result.output, container_exit_code))
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Docker returned an invalid container exit status." << std::endl;
        removeContainer(container_name, false);
        return;
    }

    removeContainer(container_name, false);
    if (container_exit_code != 0)
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Office container exited abnormally with code "
                  << container_exit_code << "; no successor capsule will be generated." << std::endl;
        return;
    }

    std::error_code filesystem_error;
    const bool valid_file = std::filesystem::is_regular_file(office_file_path, filesystem_error) &&
                            !filesystem_error &&
                            std::filesystem::file_size(office_file_path, filesystem_error) > 0 &&
                            !filesystem_error;
    if (!valid_file)
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Edited Office file is missing or empty." << std::endl;
        return;
    }

    std::string new_dc_id;
    if (!produceDataCapsule(data_capsule_info, office_file_path, new_dc_id))
    {
        std::cerr << "[OfficeOperation-handleContainerExit]: Failed to generate a successor Data Capsule." << std::endl;
        return;
    }

    if (new_dc_id.empty())
    {
        std::cout << "[OfficeOperation-handleContainerExit]: Office policy exhausted; no new Data Capsule generated."
                  << std::endl;
        return;
    }

    std::cout << "[OfficeOperation-handleContainerExit]: Successor Data Capsule generated: "
              << maskIdentifier(new_dc_id) << std::endl;
}
} // namespace

bool OfficeOperation::createOfficeFile(
    const std::string &dc_id,
    const std::string &file_extension,
    const std::string &data)
{
    office_file_path_.clear();
    file_extension_.clear();

    if (!isSafeDcId(dc_id))
    {
        std::cerr << "[OfficeOperation-createOfficeFile]: Invalid data capsule ID." << std::endl;
        return false;
    }
    if (!isAllowedOfficeExtension(file_extension))
    {
        std::cerr << "[OfficeOperation-createOfficeFile]: Unsupported Office file extension." << std::endl;
        return false;
    }
    if (data.empty())
    {
        std::cerr << "[OfficeOperation-createOfficeFile]: Office file data is empty." << std::endl;
        return false;
    }

    try
    {
        const std::filesystem::path office_directory = std::filesystem::absolute("officeFiles");
        std::error_code filesystem_error;
        std::filesystem::create_directories(office_directory, filesystem_error);
        if (filesystem_error)
        {
            std::cerr << "[OfficeOperation-createOfficeFile]: Failed to create officeFiles directory." << std::endl;
            return false;
        }

        const std::filesystem::path office_path = office_directory / (dc_id + file_extension);
        std::ofstream output(office_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
        {
            std::cerr << "[OfficeOperation-createOfficeFile]: Failed to open Office output file." << std::endl;
            return false;
        }
        output.write(data.data(), static_cast<std::streamsize>(data.size()));
        output.flush();
        if (!output.good())
        {
            output.close();
            std::filesystem::remove(office_path, filesystem_error);
            std::cerr << "[OfficeOperation-createOfficeFile]: Failed to write Office file data." << std::endl;
            return false;
        }
        output.close();

        const auto written_size = std::filesystem::file_size(office_path, filesystem_error);
        if (filesystem_error || written_size != data.size())
        {
            std::filesystem::remove(office_path, filesystem_error);
            std::cerr << "[OfficeOperation-createOfficeFile]: Office file size verification failed." << std::endl;
            return false;
        }

        // The container's limited_user may not share the Consumer host UID/GID.
        // Grant write access only to this per-capsule sandbox file so LibreOffice can save it.
        if (chmod(office_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) != 0)
        {
            std::filesystem::remove(office_path, filesystem_error);
            std::cerr << "[OfficeOperation-createOfficeFile]: Failed to make the Office file writable in the sandbox."
                      << std::endl;
            return false;
        }

        office_file_path_ = office_path.string();
        file_extension_ = file_extension;
        std::cout << "[OfficeOperation-createOfficeFile]: Office file created for capsule "
                  << maskIdentifier(dc_id) << "." << std::endl;
        return true;
    }
    catch (const std::exception &)
    {
        std::cerr << "[OfficeOperation-createOfficeFile]: Unexpected filesystem error." << std::endl;
        return false;
    }
}

bool OfficeOperation::executeOfficeOperation(const json &data_capsule_info, std::string &result)
{
    result.clear();

    if (office_file_path_.empty() || file_extension_.empty())
    {
        result = "Office operation failed: Office file path is empty.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Office file path is empty." << std::endl;
        return false;
    }

    std::error_code filesystem_error;
    if (!std::filesystem::is_regular_file(office_file_path_, filesystem_error) || filesystem_error)
    {
        result = "Office operation failed: Office file is missing.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Office file is missing." << std::endl;
        return false;
    }

    std::string policy_error;
    if (!validateCurrentOfficePolicy(data_capsule_info, policy_error))
    {
        result = "Office operation failed: " + policy_error + ".";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: " << policy_error << "." << std::endl;
        return false;
    }

    const int office_type = data_capsule_info["OfficeOperation"]["Type"].get<int>();
    if (!extensionMatchesType(file_extension_, office_type))
    {
        result = "Office operation failed: Office file extension does not match policy Type.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: File extension does not match policy Type."
                  << std::endl;
        return false;
    }

    const int host_port = kOfficeHostPort;
    if (!isPortAvailable(host_port))
    {
        result = "Office operation failed: Fixed Xpra port " +
                 std::to_string(host_port) + " is already in use.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Fixed Xpra port "
                  << host_port << " is already in use."
                  << std::endl;
        return false;
    }

    const std::string password = generateRandomString(24);
    const std::string container_name = "datacapsule-office-" + generateRandomString(16);
    const std::string container_file_path = "/home/limited_user/datacapsule" + file_extension_;
    const std::string port_mapping = std::to_string(host_port) + ":" + std::to_string(kContainerXpraPort);
    const std::string bind_option =
        "--bind-tcp=0.0.0.0:" + std::to_string(kContainerXpraPort) +
        ",auth=env:name=XPRA_PASSWORD";
    const std::string start_child =
        std::string("--start-child=") + kOfficeExecutable +
        " --nologo --nodefault --nofirststartwizard --norestore " + container_file_path;

    const std::vector<std::string> docker_command = {
        "docker",
        "run",
        "--detach",
        "--name",
        container_name,
        "--publish",
        port_mapping,
        "--mount",
        "type=bind,src=" + office_file_path_ + ",dst=" + container_file_path,
        "--env",
        "XPRA_PASSWORD",
        "--entrypoint",
        "xpra",
        kOfficeImage,
        "start",
        ":100",
        bind_option,
        "--daemon=no",
        "--mdns=no",
        "--exit-with-children=yes",
        "--terminate-children=yes",
        start_child};

    const CommandResult docker_result = runCommand(
        docker_command,
        {{"XPRA_PASSWORD", password}});
    if (docker_result.exit_code != 0)
    {
        result = "Office operation failed: Docker could not start the Office sandbox.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Docker failed to start container "
                  << container_name << "." << std::endl;
        removeContainer(container_name, true);
        return false;
    }

    if (!waitForXpraPort(host_port))
    {
        result = "Office operation failed: Xpra did not become ready.";
        const CommandResult log_result = runCommand({"docker", "logs", "--tail", "80", container_name});
        const std::string diagnostic = sanitizedDiagnostic(
            log_result.output,
            {password, office_file_path_});
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Container " << container_name
                  << " did not expose a ready Xpra port." << std::endl;
        if (!diagnostic.empty())
        {
            std::cerr << "[OfficeOperation-executeOfficeOperation]: Sanitized container diagnostic:\n"
                      << diagnostic << std::endl;
        }
        removeContainer(container_name, true);
        return false;
    }

    try
    {
        std::thread(
            handleContainerExit,
            container_name,
            office_file_path_,
            data_capsule_info)
            .detach();
    }
    catch (const std::exception &)
    {
        result = "Office operation failed: Container monitoring could not start.";
        std::cerr << "[OfficeOperation-executeOfficeOperation]: Failed to start container monitor thread."
                  << std::endl;
        removeContainer(container_name, true);
        return false;
    }

    std::ostringstream connection_information;
    connection_information << "To connect to the office file, use the Xpra client: "
                           << "xpra attach tcp://" << kAdvertisedXpraHost << ":" << host_port << "/\n"
                           << "Password: " << password;
    result = connection_information.str();

    std::cout << "[OfficeOperation-executeOfficeOperation]: Office session is ready on port "
              << host_port << "." << std::endl;
    return true;
}
