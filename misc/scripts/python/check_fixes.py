import argparse
import yaml

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Check clang-tidy fixes and throw an error if any are found"
    )
    parser.add_argument("fixes_path", help="Path to fixes exported from clang-tidy")
    args = parser.parse_args()

    with open(args.fixes_path, "r") as file:
        fixes = yaml.safe_load(file)

    diagnostics = fixes.get("Diagnostics", [])
    if len(diagnostics) > 0:
        print("Found warnings in clang-tidy export, please fix them.")
        exit(1)

    print("No warnings found in clang-tidy export.")
    exit(0)
