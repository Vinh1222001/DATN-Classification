#!/bin/bash

read -p "Component name: " COMPONENT_NAME

INCLUDE_DIR=include
SOURCE_DIR=src

# Check if the component name is exist
if test -f "${INCLUDE_DIR}/${COMPONENT_NAME}.hpp"; then 
    echo "${INCLUDE_DIR}/${COMPONENT_NAME}.hpp exists".
    exit 1
fi

if test -f "${SOURCE_DIR}/${COMPONENT_NAME}.cpp"; then 
    echo "${SOURCE_DIR}/${COMPONENT_NAME}.cpp exists".
    exit 1
fi


# Create file .hpp
cat > ${INCLUDE_DIR}/${COMPONENT_NAME}.hpp << EOF
#pragma once
#ifndef ${COMPONENT_NAME^^}_HPP
#define ${COMPONENT_NAME^^}_HPP


#endif
EOF


# Create file .cpp
cat > ${SOURCE_DIR}/${COMPONENT_NAME}.cpp << EOF
#include "${COMPONENT_NAME}.hpp" 

EOF

# Check if these files is created successfully
if test -f "${INCLUDE_DIR}/${COMPONENT_NAME}.hpp"; then 
    echo "Created ${INCLUDE_DIR}/${COMPONENT_NAME}.hpp successfully".
fi

if test -f "${SOURCE_DIR}/${COMPONENT_NAME}.cpp"; then 
    echo "Created ${SOURCE_DIR}/${COMPONENT_NAME}.cpp successfully".
fi

exit 0