# Example 1

For smooth debugging/running from IDE:

* Copy `PropertySheet.props.sample` into `PropertySheet.props`, then update the `APPDIR` and `APPEXE`.
* Copy `Example1.vcxproj.user.sample` into `Example1.vcxproj.user`.

To itegrate with Python:

Add in `PropertySheet.props`:

```xml
<PYTHON_INCLUDES>C:\Python311-32\Include;C:\Python311-32\Lib\site-packages\pybind11\include</PYTHON_INCLUDES>
<PYTHON_LIBS>C:\Python311-32\libs</PYTHON_LIBS>
<PYTHON_LIB>python311.lib</PYTHON_LIB>
```

In vcxproj, add:

* ClCompile.AdditionalIncludeDirectories: $(PYTHON_INCLUDES)
* Link.AdditionalLibraryDirectories: $(PYTHON_LIBS)
* Link.AdditionalDependencies: $(PYTHON_LIB)$

Then you need to initialize, wether find WinMain hook to init:

```cpp
#include <pybind11/embed.h> // Essential header for embedding
namespace py = pybind11;

int WINAPI hookWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	py::scoped_interpreter guard{};

	py::module_ sys = py::module_::import("sys");
    sys.attr("path").attr("append")("."); // Add current working directory

    py::module_ os = py::module_::import("os");
    std::string cwd = os.attr("getcwd")().cast<std::string>();
    
    py::dict globalScope = py::globals();

    py::exec(R"()", globalScope);

	int ret = origWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);

	return ret;
}
