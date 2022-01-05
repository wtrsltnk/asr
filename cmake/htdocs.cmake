
function(add_htdocs_file path VAR)
    file (STRINGS ${path} TMP NEWLINE_CONSUME)
    string(REPLACE "\;" ";" FIXED ${TMP})
    set(${VAR} ${FIXED} PARENT_SCOPE)
endfunction()
