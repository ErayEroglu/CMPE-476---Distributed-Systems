#!/bin/bash

# CMPE476 Distributed Systems - Submission Preparation Script

echo "=============================================="
echo "CMPE476 Project Submission Preparation"
echo "=============================================="

PROJECT_NAME="CMPE476_DistributedSystems_$(date +%Y%m%d)"
SUBMISSION_DIR="submission_$PROJECT_NAME"

# Create submission directory
echo "Creating submission directory: $SUBMISSION_DIR"
mkdir -p "$SUBMISSION_DIR"

# List of required files
REQUIRED_FILES=(
    "watchdog.c"
    "load_balancer.c" 
    "reverse_proxy.c"
    "server.c"
    "client.c"
    "Makefile"
    "report.txt"
)

# Optional documentation files
OPTIONAL_FILES=(
    "README.md"
    "TESTING_GUIDE.md"
)

echo ""
echo "Copying required files..."

# Copy required files
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        cp "$file" "$SUBMISSION_DIR/"
        echo "✅ $file"
    else
        echo "❌ Missing required file: $file"
        exit 1
    fi
done

# Copy optional files if they exist
echo ""
echo "Copying optional documentation..."
for file in "${OPTIONAL_FILES[@]}"; do
    if [ -f "$file" ]; then
        cp "$file" "$SUBMISSION_DIR/"
        echo "✅ $file (optional)"
    fi
done

# Verify report.txt has been updated
echo ""
echo "Checking report.txt..."
if grep -q "\[Your Name\]" "$SUBMISSION_DIR/report.txt"; then
    echo "⚠️  Please update report.txt with your actual name!"
    echo "   Edit $SUBMISSION_DIR/report.txt and replace [Your Name] and [Current Date]"
fi

if grep -q "\[Current Date\]" "$SUBMISSION_DIR/report.txt"; then
    echo "⚠️  Please update report.txt with the current date!"
fi

# Test compilation in submission directory
echo ""
echo "Testing compilation in submission directory..."
cd "$SUBMISSION_DIR"

if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    echo "✅ Compilation successful in submission directory"
    make clean > /dev/null 2>&1  # Clean up
else
    echo "❌ Compilation failed in submission directory"
    cd ..
    exit 1
fi

cd ..

# Create ZIP file
echo ""
echo "Creating ZIP file..."
ZIP_FILE="${PROJECT_NAME}.zip"

if command -v zip > /dev/null 2>&1; then
    zip -r "$ZIP_FILE" "$SUBMISSION_DIR"
    echo "✅ Created $ZIP_FILE"
else
    echo "⚠️  zip command not found. Creating tar.gz instead..."
    tar -czf "${PROJECT_NAME}.tar.gz" "$SUBMISSION_DIR"
    echo "✅ Created ${PROJECT_NAME}.tar.gz"
fi

# Show file sizes
echo ""
echo "Submission contents:"
ls -la "$SUBMISSION_DIR"

echo ""
echo "Archive created:"
ls -lh "$ZIP_FILE" 2>/dev/null || ls -lh "${PROJECT_NAME}.tar.gz" 2>/dev/null

# Final checklist
echo ""
echo "=============================================="
echo "SUBMISSION CHECKLIST"
echo "=============================================="
echo "✅ All required source files included"
echo "✅ Makefile included"
echo "✅ Compilation tested"
echo ""
echo "Before submitting, please verify:"
echo "[ ] Updated report.txt with your name and date"
echo "[ ] Tested the system thoroughly"
echo "[ ] Reviewed all LLM prompts in report.txt"
echo "[ ] ZIP file is under size limit (if any)"
echo ""
echo "Ready for submission to Moodle!"
echo ""
echo "Files to submit:"
if [ -f "$ZIP_FILE" ]; then
    echo "  📁 $ZIP_FILE"
else
    echo "  📁 ${PROJECT_NAME}.tar.gz"
fi

# Cleanup
echo ""
echo "Cleaning up temporary submission directory..."
rm -rf "$SUBMISSION_DIR"
echo "✅ Cleanup complete"

echo ""
echo "🎉 Submission preparation complete!" 