#!/bin/bash

# Clone the repository
clone_repo() {
    if [ ! -d "portal_turret_webpage" ]; then
        git clone https://github.com/joranderaaff/portal_turret_webpage.git
    fi
}

# Install npm dependencies
install_dependencies() {
    if [ ! -d "portal_turret_webpage/node_modules" ]; then
        clone_repo
        cd portal_turret_webpage
        npm install
        cd ..
    fi
}

# Build the project
build_project() {
    install_dependencies
    cd portal_turret_webpage
    npm run build
    cp dist_compiled/index.html.gz.h ../generated/
    cd ..
}

# Clean up the project
distclean() {
    rm -rf portal_turret_webpage
}

# Main function to control the flow
main() {
    case "$1" in
        distclean)
            distclean
            ;;
        *)
            build_project
            ;;
    esac
}

# Execute main function with passed arguments
main "$@"