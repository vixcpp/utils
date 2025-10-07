# ✅ Pull Request Template – `vixcpp/utils`

## 📄 Description

Please include a clear and concise summary of the changes in this PR,  
along with the motivation behind them (e.g., utility improvements, new helpers, refactoring, etc.).

Reference any related issues or feature requests:  
**Fixes** # (issue number)

---

## 🔍 Type of Change

Please check all that apply:

- [ ] 🐞 **Bug fix** (non-breaking change that fixes an issue)
- [ ] ✨ **New feature** (non-breaking change that adds functionality)
- [ ] 💥 **Breaking change** (modifies existing functionality or API)
- [ ] 📝 **Documentation update**
- [ ] ✅ **Tests / CI improvements**

---

## 🧪 How Has This Been Tested?

Please describe the tests you ran to verify your changes.  
Include steps for others to reproduce them if needed:

```bash
# Create and enter build directory
mkdir build && cd build

# Configure the build system
cmake ..

# Build all targets
make -j$(nproc)
```

If unit tests were added (recommended for utility functions):

# Run utility tests

```bash
./test_utils
```

Mention what specific utility headers/functions were tested (e.g., trim(), read_file(), get_current_timestamp()).

✅ Checklist

My code follows the style guidelines of this project

I have reviewed my own code

I have added comments where needed

I have updated relevant documentation

I have added unit tests where appropriate

All new and existing tests pass

🧩 Additional Notes

Include any other relevant information here, such as:

Edge cases handled

Benchmark results for performance-sensitive utilities

Screenshots or logs for file/path utilities

Limitations or TODOs for future improvements
