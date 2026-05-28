# GR4 Block Library Testing

This is a custom block library created to test and develop blocks while strictly adhering to the standards and structure of the GNU Radio 4 block libraries, as defined in the [gnuradio4 repository](https://github.com/fair-acc/gnuradio4/tree/main/blocks).

## Repository Structure

Here is an overview of the project's directory structure and what each component does:

* **`custom_tests/include/gnuradio-4.0/custom_tests/`**
  Contains the header files for defining the test blocks.

* **`test/`**
  Directory dedicated to unit tests to ensure block functionality.
  * `qa_*` — Quality assurance (QA) test files for the individual blocks.
  * `CMakeLists.txt` — CMake configuration specifically for building and running the tests.

* **`src/`**
  Contains examples of block usage and implementation source code.

* **`assets/`**
  Additional documentation

* **`README.md`**
  A short description and guide for the block library (this file).
