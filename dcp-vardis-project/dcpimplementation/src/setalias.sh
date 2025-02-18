alias do-cmake='cmake -S . -B ../_build'
alias do-build='cmake --build ../_build'
alias do-clean='rm -rf ../_build/*'
alias do-exec-bp='../_build/dcp-bp'
alias do-exec-srp='../_build/dcp-srp'
alias do-test='cd ../_build/ && ctest && cd ../src/'
