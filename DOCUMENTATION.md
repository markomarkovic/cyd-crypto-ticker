# CYD Crypto Ticker - Documentation Guide

## Overview

This document provides a comprehensive guide to the documentation system for the CYD Crypto Ticker project. The project uses **Doxygen** to generate detailed HTML documentation from comments embedded in the source code.

## Documentation Structure

### Generated Documentation

The project includes comprehensive Doxygen documentation covering:

- **All Classes**: Complete class documentation with inheritance diagrams
- **All Functions**: Detailed function documentation with parameters and return values
- **All Variables**: Variable documentation with descriptions and usage notes
- **Constants**: Hardware pins, colors, timing intervals, and configuration values
- **Modules**: Logical groupings of related functionality
- **File Organization**: Project structure and file relationships

### Documentation Categories

#### 1. **Core Application Classes**

- `ApplicationController` - Main application orchestrator
- `BinanceDataManager` - Cryptocurrency data management
- `WebSocketManager` - Real-time WebSocket connections
- `NetworkManager` - WiFi and web configuration
- `DisplayManager` - LVGL-based user interface
- `HardwareController` - Hardware peripheral control
- `ApplicationStateManager` - Application state tracking
- `StatusCalculator` - Performance calculations

#### 2. **Data Structures**

- `CoinData` - Cryptocurrency information structure
- Various enums for application states and connection status

#### 3. **Configuration Constants**

- **Colors**: UI theme colors and muted colors for disconnected state
- **Hardware Pins**: GPIO assignments for LEDs, sensors, buttons
- **Timing**: Intervals for updates, reconnections, and timeouts
- **Logging**: Log levels and safe serial communication macros
- **WebSocket**: Connection parameters and retry configuration

#### 4. **Logging System**

- **Log Levels**: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- **Safe Macros**: Non-blocking serial communication with timeout protection
- **Legacy Support**: Backward compatibility for existing code

## Generating Documentation

### Prerequisites

1. **Doxygen**: Install Doxygen on your system
   ```bash
   # Ubuntu/Debian
   sudo apt-get install doxygen
   
   # macOS
   brew install doxygen
   
   # Windows
   # Download from https://www.doxygen.nl/download.html
   ```

### Using the Documentation Script

The project includes an automated documentation generation script:

```bash
# Generate documentation
./generate_docs.sh

# Clean existing documentation
./generate_docs.sh clean

# Generate documentation and open in browser
./generate_docs.sh open

# Show help
./generate_docs.sh help
```

### Manual Generation

You can also generate documentation manually:

```bash
# Run Doxygen with the provided configuration
doxygen Doxyfile
```

## Documentation Configuration

### Doxyfile Settings

The project includes a pre-configured `Doxyfile` with optimized settings:

- **Input Sources**: `src/`, `include/`, `CLAUDE.md`, `README.md`
- **Output Format**: HTML (with optional LaTeX/PDF)
- **Extraction**: All documented and undocumented members
- **Features**: Source browsing, class diagrams, call graphs
- **Exclusions**: `.pio/` directory, `boards/`, `test/`

### Key Configuration Options

```ini
PROJECT_NAME           = "CYD Crypto Ticker"
PROJECT_BRIEF          = "Real-time cryptocurrency price display for ESP32-based CYD boards"
OUTPUT_DIRECTORY       = docs/
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = YES
SOURCE_BROWSER         = YES
GENERATE_HTML          = YES
CLASS_DIAGRAMS         = YES
```

## Viewing Documentation

After generation, documentation is available at:
- **Main Index**: `docs/html/index.html`
- **Class List**: `docs/html/annotated.html`
- **File List**: `docs/html/files.html`
- **Module List**: `docs/html/modules.html`

### Navigation Features

The generated documentation includes:
- **Search functionality** for quick navigation
- **Class inheritance diagrams** showing relationships
- **Call graphs** for function dependencies
- **Source code browsing** with syntax highlighting
- **Cross-references** between related items

## Documentation Standards

### Comment Style

The project uses Javadoc-style comments:

```cpp
/**
 * @brief Brief description of the function
 * 
 * Detailed description explaining what the function does,
 * how it works, and any important notes about usage.
 * 
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * 
 * @see RelatedClass
 * @note Important usage notes
 * @warning Warnings about potential issues
 */
int exampleFunction(int param1, float param2);
```

### Documentation Tags

Common Doxygen tags used in the project:

- `@file` - File description
- `@class` - Class description
- `@brief` - Brief description
- `@param` - Parameter description
- `@return` - Return value description
- `@see` - Cross-references
- `@note` - Important notes
- `@warning` - Warnings
- `@deprecated` - Deprecated items
- `@defgroup` - Module groupings

### Module Organization

The documentation is organized into logical modules:

```cpp
/**
 * @defgroup Colors Color Definitions
 * @brief Color constants for LVGL display theming
 * @{
 */

// Color definitions go here

/** @} */ // end of Colors group
```

## Contributing to Documentation

### Adding Documentation

When adding new code:

1. **Document all public interfaces** with detailed descriptions
2. **Include parameter and return value descriptions**
3. **Add usage examples** for complex functions
4. **Document any side effects or preconditions**
5. **Include cross-references** to related functionality

### Updating Documentation

When modifying existing code:

1. **Update corresponding documentation** to match changes
2. **Verify documentation accuracy** after modifications
3. **Check for broken cross-references**
4. **Regenerate documentation** to verify formatting

### Documentation Review

Before committing changes:

1. **Generate documentation locally** to check for warnings
2. **Review generated HTML** for formatting issues
3. **Verify cross-references work correctly**
4. **Check that new modules are properly organized**

## Integration with Development Workflow

### IDE Integration

Many IDEs can display Doxygen comments:
- **VS Code**: Install Doxygen extensions
- **CLion**: Built-in Doxygen support
- **Eclipse CDT**: Doxygen plugin available

### Continuous Integration

Consider integrating documentation generation into CI/CD:

```yaml
# Example GitHub Actions workflow
- name: Generate Documentation
  run: |
    sudo apt-get install doxygen
    ./generate_docs.sh
    
- name: Deploy Documentation
  uses: peaceiris/actions-gh-pages@v3
  with:
    github_token: ${{ secrets.GITHUB_TOKEN }}
    publish_dir: ./docs/html
```

## Troubleshooting

### Common Issues

1. **Missing Doxygen**: Install Doxygen package
2. **Permission Issues**: Make script executable with `chmod +x generate_docs.sh`
3. **Empty Documentation**: Check file paths in Doxyfile
4. **Broken Links**: Verify cross-references use correct names

### Documentation Warnings

Doxygen may produce warnings for:
- Undocumented parameters
- Missing return value descriptions
- Broken cross-references
- Invalid command syntax

Address warnings to improve documentation quality.

## Advanced Features

### Custom CSS Styling

Customize documentation appearance:
```ini
HTML_EXTRA_STYLESHEET = custom.css
```

### Diagram Generation

Enable advanced diagrams (requires Graphviz):
```ini
HAVE_DOT = YES
CLASS_GRAPH = YES
COLLABORATION_GRAPH = YES
CALL_GRAPH = YES
```

### LaTeX/PDF Output

Generate PDF documentation:
```ini
GENERATE_LATEX = YES
USE_PDFLATEX = YES
PDF_HYPERLINKS = YES
```

## Maintenance

### Regular Tasks

1. **Update documentation** when adding new features
2. **Review and fix warnings** during builds
3. **Regenerate documentation** before releases
4. **Check cross-references** remain valid
5. **Update this guide** as the project evolves

### Quality Assurance

- **Spell-check documentation** content
- **Verify technical accuracy** of descriptions
- **Test example code** mentioned in documentation
- **Ensure consistency** in terminology and style

## Resources

- **Doxygen Manual**: https://www.doxygen.nl/manual/
- **Doxygen Commands**: https://www.doxygen.nl/manual/commands.html
- **Best Practices**: https://www.doxygen.nl/manual/docblocks.html

---

For questions about the documentation system or to suggest improvements, please refer to the project's main documentation in `CLAUDE.md` or create an issue in the project repository.