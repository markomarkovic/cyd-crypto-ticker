#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

// File paths
const CSS_FILE = path.join(__dirname, 'css', 'minimal.min.css');
const CPP_FILE = path.join(__dirname, '..', 'src', 'NetworkManager.cpp');

// CSS markers in the C++ file
const CSS_START_MARKER = '<style>';
const CSS_END_MARKER = '</style>';

function updateCppFile() {
    try {
        // Read the minified CSS
        const cssContent = fs.readFileSync(CSS_FILE, 'utf8').trim();
        console.log(`✓ Read CSS file: ${cssContent.length} bytes`);

        // Read the C++ file
        const cppContent = fs.readFileSync(CPP_FILE, 'utf8');
        console.log(`✓ Read C++ file: ${CPP_FILE}`);

        // Find the CSS section in the C++ file
        const startIndex = cppContent.indexOf(CSS_START_MARKER);
        const endIndex = cppContent.indexOf(CSS_END_MARKER, startIndex);

        if (startIndex === -1 || endIndex === -1) {
            throw new Error(`Could not find CSS markers (${CSS_START_MARKER} ... ${CSS_END_MARKER}) in NetworkManager.cpp`);
        }

        // Replace the CSS content
        const beforeCSS = cppContent.substring(0, startIndex + CSS_START_MARKER.length);
        const afterCSS = cppContent.substring(endIndex);
        const newContent = beforeCSS + cssContent + afterCSS;

        // Write the updated C++ file
        fs.writeFileSync(CPP_FILE, newContent);
        
        console.log(`✓ Updated NetworkManager.cpp with new CSS`);
        console.log(`  CSS size: ${cssContent.length} bytes`);
        console.log(`  File: ${CPP_FILE}`);
        
    } catch (error) {
        console.error('❌ Error updating C++ file:', error.message);
        process.exit(1);
    }
}

// Run the update
console.log('🔄 Updating NetworkManager.cpp with latest CSS...');
updateCppFile();
console.log('✅ CSS update complete!');