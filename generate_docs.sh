#!/bin/bash

##
# @file generate_docs.sh
# @brief Automatic documentation generation script for CYD Crypto Ticker project
# @author Claude Code Assistant  
# @date 2024
#
# This script automatically generates HTML documentation from Doxygen comments
# in the source code. It checks for Doxygen installation, cleans previous
# documentation, and generates fresh HTML documentation.
#
# Usage:
#   ./generate_docs.sh          # Generate documentation
#   ./generate_docs.sh clean    # Clean documentation only
#   ./generate_docs.sh open     # Generate and open in browser
##

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project configuration
PROJECT_NAME="CYD Crypto Ticker"
DOCS_DIR="docs"
HTML_DIR="$DOCS_DIR/html"
DOXYFILE="Doxyfile"

##
# @brief Print colored status message
# @param color Color code for the message
# @param message Message to print
##
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}[INFO]${NC} $message"
}

##
# @brief Print error message and exit
# @param message Error message to print
##
print_error() {
    local message=$1
    echo -e "${RED}[ERROR]${NC} $message" >&2
    exit 1
}

##
# @brief Check if required tools are installed
##
check_dependencies() {
    print_status $BLUE "Checking dependencies..."
    
    if ! command -v doxygen &> /dev/null; then
        print_error "Doxygen is not installed. Please install it first."
    fi
    
    print_status $GREEN "Doxygen found: $(doxygen --version)"
}

##
# @brief Clean existing documentation
##
clean_docs() {
    print_status $YELLOW "Cleaning existing documentation..."
    
    if [ -d "$DOCS_DIR" ]; then
        rm -rf "$DOCS_DIR"
        print_status $GREEN "Cleaned documentation directory"
    else
        print_status $BLUE "No existing documentation to clean"
    fi
}

##
# @brief Generate documentation using Doxygen
##
generate_docs() {
    print_status $BLUE "Generating documentation for $PROJECT_NAME..."
    
    if [ ! -f "$DOXYFILE" ]; then
        print_error "Doxyfile not found. Please ensure it exists in the project root."
    fi
    
    # Create docs directory if it doesn't exist
    mkdir -p "$DOCS_DIR"
    
    # Generate documentation
    if doxygen "$DOXYFILE"; then
        print_status $GREEN "Documentation generated successfully!"
        print_status $BLUE "HTML documentation available at: $HTML_DIR/index.html"
    else
        print_error "Documentation generation failed"
    fi
}

##
# @brief Open documentation in default browser
##
open_docs() {
    local index_file="$HTML_DIR/index.html"
    
    if [ ! -f "$index_file" ]; then
        print_error "Documentation not found. Generate it first."
    fi
    
    print_status $BLUE "Opening documentation in browser..."
    
    # Detect OS and open appropriately
    if command -v xdg-open &> /dev/null; then
        # Linux
        xdg-open "$index_file"
    elif command -v open &> /dev/null; then
        # macOS
        open "$index_file"
    elif command -v start &> /dev/null; then
        # Windows (Git Bash, WSL)
        start "$index_file"
    else
        print_status $YELLOW "Cannot detect browser opener. Please manually open: $index_file"
    fi
}

##
# @brief Display usage information
##
show_usage() {
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Generate HTML documentation for $PROJECT_NAME using Doxygen"
    echo ""
    echo "Options:"
    echo "  (none)    Generate documentation (default)"
    echo "  clean     Clean existing documentation only"
    echo "  open      Generate documentation and open in browser"
    echo "  help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0           # Generate documentation"
    echo "  $0 clean     # Clean documentation"
    echo "  $0 open      # Generate and open in browser"
}

##
# @brief Main script execution
##
main() {
    echo "=========================================="
    echo " $PROJECT_NAME Documentation Generator"
    echo "=========================================="
    echo ""
    
    # Parse command line arguments
    case "${1:-}" in
        "clean")
            clean_docs
            ;;
        "open")
            check_dependencies
            clean_docs
            generate_docs
            open_docs
            ;;
        "help"|"-h"|"--help")
            show_usage
            exit 0
            ;;
        "")
            # Default action - generate docs
            check_dependencies
            clean_docs
            generate_docs
            ;;
        *)
            print_error "Unknown option: $1. Use 'help' for usage information."
            ;;
    esac
    
    echo ""
    print_status $GREEN "Documentation generation complete!"
}

# Run main function with all arguments
main "$@"