import os
import subprocess

def get_file_list(root_path: str, extensions: list, skip_dirs: list = ["ThirdParty"]):
    all_files = []
    for dirpath, _, filenames in os.walk(root_path):
        if any(skip_dir in dirpath for skip_dir in skip_dirs):
            continue

        for filename in filenames:
            if any(filename.endswith(ext) for ext in extensions):
                all_files.append(os.path.join(dirpath, filename))
    return all_files


if __name__ == "__main__":
    main_directory = os.path.abspath(
        os.path.join(os.path.dirname(__file__), ".."))
    os.chdir(main_directory)

    clang_format_exe = r"C:\work\bin\clang-format.exe"
    style_file_path = os.path.abspath(".clang-format")

    extensions = [".c", ".cpp", ".h", ".inl"]
    directories = [
        r"src\Core",
        r"src\RenderSystem",
        r"src\Scene",
        r"include",
    ]

    all_files = []

    for directory in directories:
        all_files += get_file_list(directory, extensions)

    for file_path in all_files:
        print(f"Formatting file: {file_path}")
        abs_path = os.path.abspath(file_path)
        cmd_line = f'"{clang_format_exe}" -i "{abs_path}" --style=file:"{style_file_path}"'
        subprocess.call(cmd_line, shell=True)
