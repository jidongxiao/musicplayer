#include <iostream>
#include <string>

int run_cli();
#ifdef CLI_ONLY
int run_gui(int argc, char* argv[])
{
    std::cerr << "GUI not available in CLI build.\n";
    return 1;
}
#else
int run_gui(int argc, char* argv[]);
#endif

int main(int argc, char* argv[])
{
    // Default to GUI if no argument is given
    if (argc < 2) {
	return run_gui(argc, argv);
    }

    std::string mode = argv[1];

    if (mode == "--cli") {
        return run_cli();
    }
    else if (mode == "--gui") {
        return run_gui(argc, argv);
    }
    else {
        std::cerr << "Unknown option: " << mode << "\n";
        return 1;
    }
}
