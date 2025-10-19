#!/usr/bin/env python3
import json
import sys
from pathlib import Path

def validate_package_json(filepath):
    """Validate a package JSON file"""
    required_fields = [
        'name', 'version', 'description', 'category', 'license',
        'maintainer', 'repository', 'architectures', 'sources',
        'binaries', 'dependencies', 'mode', 'commands'
    ]
    
    with open(filepath) as f:
        data = json.load(f)
    
    # Check required fields
    missing = [field for field in required_fields if field not in data]
    if missing:
        print(f"❌ {filepath}: Missing required fields: {', '.join(missing)}")
        return False
    
    # Validate SHA256 checksums format
    def check_sha256(obj, path=""):
        if isinstance(obj, dict):
            if 'sha256' in obj:
                sha = obj['sha256']
                if not isinstance(sha, str) or len(sha) != 64:
                    print(f"❌ {filepath}: Invalid SHA256 at {path}")
                    return False
            for key, value in obj.items():
                if not check_sha256(value, f"{path}.{key}"):
                    return False
        elif isinstance(obj, list):
            for i, item in enumerate(obj):
                if not check_sha256(item, f"{path}[{i}]"):
                    return False
        return True
    
    if not check_sha256(data):
        return False
    
    print(f"✅ {filepath}: Valid")
    return True

def validate_index_json(filepath):
    """Validate the main index.json"""
    with open(filepath) as f:
        data = json.load(f)
    
    required_fields = ['version', 'repository', 'packages', 'total']
    missing = [field for field in required_fields if field not in data]
    if missing:
        print(f"❌ {filepath}: Missing required fields: {', '.join(missing)}")
        return False
    
    print(f"✅ {filepath}: Valid")
    return True

def main():
    repo_root = Path(__file__).parent.parent
    
    valid = True
    
    # Validate index.json
    index_file = repo_root / 'index.json'
    if index_file.exists():
        valid = validate_index_json(index_file) and valid
    
    # Validate all package JSONs
    packages_dir = repo_root / 'packages'
    if packages_dir.exists():
        for json_file in packages_dir.glob('*.json'):
            valid = validate_package_json(json_file) and valid
    
    if not valid:
        sys.exit(1)
    
    print("\n🎉 All JSON files are valid!")

if __name__ == '__main__':
    main()