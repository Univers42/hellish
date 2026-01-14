#!/bin/bash

MINISHELL="../minishell"

run_echo2() {
    INPUT="$1"
    EXPECTED="$2"

    printf "%s\n" "$INPUT" | $MINISHELL > /dev/null
    eval "$EXPECTED" > temp2
    DISPLAY=$(echo "$INPUT" | sed 's/ > *temp1//')
    if diff -q temp1 temp2 >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $DISPLAY"
    else
        echo -e "${RED}❌${RESET}  $DISPLAY"
        echo "     Diferencias:"
        cat temp1
        cat temp2
    fi
    rm temp1 temp2
}

run_echo() {
    INPUT="$1"
    EXPECTED="$2"

    printf "%s\n" "$INPUT" | $MINISHELL | tail -n +2 | head -n1 > temp1
    eval "$EXPECTED" > temp2
    if diff -q temp1 temp2 >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        cat temp1
        cat temp2
    fi
    rm temp1 temp2
}

run_echo_redir() {
    INPUT="$1"
    EXPECTED="$2"

    # Extrae el nombre del archivo después de >
    OUTFILE=$(echo "$INPUT" | sed -n 's/.*>\s*\([^ ]*\).*/\1/p')
    # Ejecuta el input dentro de minishell (redirección incluida)
    printf "%s\n" "$INPUT" | $MINISHELL > /dev/null
    # Ejecuta en bash para generar temp
    eval "$EXPECTED" > temp2
    # Muestra el comando limpio (sin redirección)
    DISPLAY=$(echo "$INPUT" | sed 's/ *>\s*[^ ]*//')
    # Compara la salida generada por minishell con la de bash
    if diff -q "$OUTFILE" temp2 >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $DISPLAY"
    else
        echo -e "${RED}❌${RESET}  $DISPLAY"
        echo "     Diferencias:"
        cat "$OUTFILE"
        cat temp2
    fi
    rm -f "$OUTFILE" temp2
}




# Función para limpiar el output y quedarnos con lo anterior al prompt verde
clean_output() {
    input="$1"
    full_output=$(printf "%s\n" "$input" | $MINISHELL)

    output=$(echo "$full_output" | tail -n +2)

    clean=""
    found=0

    while IFS= read -r line; do
        if [[ "$line" == *$'\033[1;32m'miniyo'$'* ]]; then
            before_prompt="${line%%$'\033[1;32m'miniyo\$*}"
            clean+="$before_prompt"
            found=1
            break
        else
            clean+="$line"$'\n'
        fi
    done <<< "$output"
    echo -n "$clean"
}

clean_output2() {
    echo "$1" | \
    grep -v 'miniyo\$' | \
    grep -v '^export ' | \
    grep -v '^echo ' | \
    grep -v '^exit$' | \
    sed '/^[[:space:]]*$/d'
}


# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0m'


run_redir() {
    INPUT="$1"
    EXPECTED="$2"

    clean_output "$INPUT" > temp1
    eval "$EXPECTED" > temp2
    if diff -q kk temp2 >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        cat kk
        cat temp2
    fi
    rm kk temp2
}

run_exit()
{
    INPUT="$1"
	EXPECTED="$2"
    
	echo "$INPUT" | ../minishell > /dev/null 2>&1
    EXIT_CODE=$?

	if [ "$EXIT_CODE" -eq "$EXPECTED" ]; then
        echo -e "${GREEN}✔️${RESET}  $INPUT (exit $EXIT_CODE)"
    else
        echo -e "${RED}❌${RESET}  $INPUT (got $EXIT_CODE, expected $EXPECTED)"
    fi
}



run()
{
    INPUT="$1"
    EXPECTED="$2"
    mkdir -p ../redirs
    # Ejecuta el comando en minishell y elimina la primera y última línea
    printf "%s\n" "$INPUT" | $MINISHELL 2>&1 | sed '1d;$d' > ../redirs/temp1
    # Ejecuta el mismo comando en bash
    bash -c "$EXPECTED" > ../redirs/temp2 2>&1
    if diff -q ../redirs/temp1 ../redirs/temp2 >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        cat ../redirs/temp1
        cat ../redirs/temp2
    fi
    rm -rf ../redirs
}

run_command_invalid()
{
	INPUT="$1"
    EXPECTED="$2"

    printf "%s\n" "$INPUT" | $MINISHELL > temp1 2>&1
    bash -c "$EXPECTED" > temp2 2>&1

    if grep -q "command not found" temp1 && grep -q "command not found" temp2; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        cat temp1
        cat temp2
    fi

    rm -f temp1 temp2
}

run_directory_invalid()
{
    INPUT="$1"
    EXPECTED="$2"

    printf "%s\n" "$INPUT" | $MINISHELL > temp1 2>&1
    bash -c "$EXPECTED" > temp2 2>&1

    if grep -q "No such file or directory" temp1 && grep -q "No such file or directory" temp2; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        cat temp1
        cat temp2
    fi

    rm -f temp1 temp2
}


run_command_return_value()
{
    INPUT="$1"
	EXPECTED="$2"
    
	echo -e "$INPUT\nexit \$?" | ../minishell > redirs/temp1 2>&1
    EXIT_CODE=$?

	if [ "$EXIT_CODE" -eq "$EXPECTED" ]; then
        echo -e "${GREEN}✔️${RESET}  $INPUT (exit $EXIT_CODE)"
    else
        echo -e "${RED}❌${RESET}  $INPUT (got $EXIT_CODE, expected $EXPECTED)"
		cat temp1
    fi
	rm redirs/temp1
}

run_appends()
{
    INPUT="$1"
    EXPECTED="$2"

    mkdir -p ../redirs

    cp ./redirs/a ../redirs/a
    cp ./redirs/b ../redirs/b

    FILENAME=$(echo "$INPUT" | sed -E 's/.*>>?\s*([^ >]+)\s*$/\1/')

    clean_output "$INPUT"  # Ejecuta minishell

    cp ./redirs/b ../redirs/c  # Prepara el archivo base para bash
    bash -c "$EXPECTED"        # Bash modifica ../redirs/c

    Compara lo que hizo la minishell con lo que hizo bash
    if diff -q ./$FILENAME ../redirs/c >/dev/null; then
        echo -e "${GREEN}✔️${RESET}  $INPUT"
    else
        echo -e "${RED}❌${RESET}  $INPUT"
        echo "     Diferencias:"
        diff ./$FILENAME ../redirs/c
    fi

    find . -type f -name "*.txt" -delete
    rm -f ./$FILENAME
    rm -rf ../redirs
}

clear

echo "##########################"
echo "# COMANDS con ruta       #"
echo "##########################"
echo -e "\n"

run '/bin/ls' '/bin/ls'
run '/bin/pwd' '/bin/pwd'
run '/bin/echo run_echo' '/bin/echo run_echo'
run_echo2 '/bin/echo -n run_echo > temp1' '/bin/echo -n run_echo'
run_echo2 '/bin/printf run_echo > temp1' '/bin/printf run_echo'
run '/bin/cat redirs/a' '/bin/cat redirs/a'
run 'whoami' 'whoami'
run '' ''
run '       ' '       '
run '/bin/ls -la' '/bin/ls -la'
run '/bin/ls -l' '/bin/ls -l'
run 'echo uno dos tres cuatro' 'echo uno dos tres cuatro'
run '/bin/ls -l -a -h' '/bin/ls -l -a -h'
run '/bin/ls -lhS' '/bin/ls -lhS'
run '/bin/cat redirs/a redirs/b' '/bin/cat redirs/a redirs/b'
run 'head -n 5 redirs/a' 'head -n 5 redirs/a'

echo -e "\n"
echo "##############"
echo "# ECHO       #"
echo "##############"
echo -e "\n"

run_echo 'echo patata' 'echo patata'
run_echo 'echo $a""a' 'echo $a""a'
run_echo 'echo $a"" a' 'echo $a"" a'
run_echo 'echo $a" " a' 'echo $a" " a'
run_echo 'echo $a "" a' 'echo $a "" a'
run_echo 'echo $a" $USER"' 'echo $a" $USER"'
run_echo 'echo $a"$USER"' 'echo $a"$USER"'
run_echo 'echo $USERa$HOME' 'echo $USERa$HOME'
run_echo 'echo "$  a"' 'echo "$  a"'
run_echo 'echo "$  a "' 'echo "$  a "'
run_echo 'echo "$?u "' 'echo "$?u "'
run_echo 'echo "$? us "' 'echo "$? us "'
export kk=-n
run_echo2 'echo $kk run_echo > temp1' 'echo $kk run_echo'
export kk="-nnn -na run_echo"
run_echo2 'echo $kk tomate > temp1' 'echo $kk tomate'
run_echo 'echo $a' 'echo $a'
run_echo 'echo $"mgs"' 'echo $"mgs"'
run_echo "echo $'msg'" "echo $'msg'"
export kk="echo -n msg1"
run_echo2 '$kk msg2 > temp1' '$kk msg2'
export kk="echo -nnnnn -n -nnnann msg1"
run_echo2 '$kk msg2 > temp1' '$kk msg2'
export kk="echo msg1"
run_echo '$kk -n msg2' '$kk -n msg2'
run_echo2 'echo ""-n run_echo > temp1' 'echo ""-n run_echo'
run_echo 'echo "" -n run_echo' 'echo "" -n run_echo'
run_echo2 'echo "-n" run_echo > temp1' 'echo "-n" run_echo'
run_echo2 "echo '-n' run_echo > temp1" "echo '-n' run_echo"
run_echo 'echo "-n run_echo"' 'echo "-n run_echo"'
run_echo "echo '-n run_echo'" "echo '-n run_echo'"
run_echo 'echo "-n -na" run_echo' 'echo "-n -na" run_echo'
run_echo2 'echo "-nnnnnn" run_echo > temp1' 'echo "-nnnnnn" run_echo'
run_echo 'echo "-nnnnn " run_echo' 'echo "-nnnnn " run_echo'
run_echo 'echo " " -n run_echo' 'echo " " -n run_echo'
run_echo 'echo " " run_echo' 'echo " " run_echo'
export kk="  hola         que     tal      "
run_echo 'echo $kk' 'echo $kk'
run_echo 'echo "$kk"' 'echo "$kk"'
run_echo "echo '\$kk'" "echo '\$kk'"
export kk="echo tomate"
run_echo '$kk' '$kk'
run_echo 'echo "$ a"' 'echo "$ a"'
run_echo 'echo "$a5e#tomate 'p'"' 'echo "$a5e#tomate 'p'"'
run_echo 'echo "$a 'p'"' 'echo "$a 'p'"'
run_echo 'echo "$%run_echo"' 'echo "$%run_echo"'
run_echo 'echo "$!run_echo"' 'echo "$!run_echo"'
run_echo 'echo "$a664464562ssertetr)'p'"' 'echo "$a664464562ssertetr)'p'"'
run_echo 'echo $2run_echo' 'echo run_echo'
run_echo 'echo $2 run_echo' 'echo run_echo'
run_echo 'echo $* run_echo' 'echo run_echo'
run_echo "echo 'run_echo"\$USER"'" "echo 'run_echo"\$USER"'"
run_echo 'echo $.' 'echo $.'
run_echo 'echo $' 'echo $'
run_echo 'echo hola' 'echo hola'
run_echo 'echo "'hola'"' 'echo "'hola'"'
run_echo "echo '"hola"'" "echo '"hola"'"
run_echo2 'echo -n hola > temp1' 'echo -n hola'
run_echo2 'echo -n -n hola > temp1' 'echo -n -n hola'
run_echo2 'echo -n -n -n hola > temp1' 'echo -n -n -n hola'
run_echo2 'echo -n -n -n -n hola > temp1' 'echo -n -n -n -n hola'
run_echo2 'echo -n -nnnn hola > temp1' 'echo -n -nnnn hola'
run_echo2 'echo -nnnn hola > temp1' 'echo -nnnn hola'
run_echo 'echo hola "\"n mundo' "echo 'hola \n mundo'"
run_echo 'echo "hola mundo"' 'echo "hola mundo"'
run_echo "echo 'hola mundo'" "echo 'hola mundo'"
run_echo 'echo hola" mundo"' 'echo hola" mundo"'
run_echo 'echo "hola mundo"' 'echo "hola mundo"'
run_echo "echo 'hola mundo'" "echo 'hola mundo'"
run_echo "echo 'hola'\" mundo\"" "echo hola mundo"
run_echo 'echo "saludo: $HOME"' 'echo "saludo: $HOME"'
export kk=32
run_echo 'echo $kk' 'echo $kk'
export kk=hola
run_echo 'echo "$kk"' 'echo "$kk"'
run_echo 'echo $NOEXISTE' 'echo $NOEXISTE'
run_echo 'echo $PATH' 'echo $PATH'
run_echo 'echo $LS_COLORS' 'echo $LS_COLORS'
run_echo 'echo "$USER$HOME"' 'echo "$USER$HOME"'
run_echo 'echo $LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH' 'echo $LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH$LS_COLORS$PATH'
run_echo 'echo "$USER"' 'echo "$USER"'
run_echo 'echo $USER' 'echo $USER'
run_echo "echo '\$USER'" "echo '\$USER'"
run_echo 'echo "$?"' 'echo "$?"'
run_echo 'echo "$USER literal $HOME"' 'echo "$USER literal $HOME"'
export UNSETVAR=run_echo
unset UNSETVAR
run_echo 'echo $UNSETVAR' 'echo $UNSETVAR'
run_echo 'echo ""' 'echo ""'
run_echo "echo ''" "echo ''"
run_echo 'echo "hola     mundo"' 'echo "hola     mundo"'
run_echo 'echo hola     mundo' 'echo hola     mundo'
run_echo2 'echo -n "hola\nmundo" > temp1' 'echo -n "hola\nmundo"'
run_echo 'echo "a'\$USER'"' 'echo "a'\$USER'"'
run_echo 'echo "'\$USER'"' 'echo "'\$USER'"'
run_echo "echo \"\$USER's folder\"" "echo \"\$USER's folder\""
run_echo 'echo "$UNDEFINED"' 'echo "$UNDEFINED"'
run_echo 'echo "$UNDEFINED texto"' 'echo "$UNDEFINED texto"'
run_echo "echo '\$UNDEFINED texto'" "echo '\$UNDEFINED texto'"
run_echo 'echo "$USER''$HOME"' 'echo "$USER''$HOME"'
run_echo 'echo "$USER '' $HOME"' 'echo "$USER '' $HOME"'
run_echo 'echo "  $USER  "' 'echo "  $USER  "'
run_echo "echo \"'single' \"double\"\"" "echo \"'single' double\""
run_echo 'echo "$? status"' 'echo "$? status"'
run_echo 'echo $?' 'echo $?'
run_echo "echo \"'\$?'\"" "echo \"'\$?'\""
run_echo "echo '\"hola\"'" "echo '\"hola\"'"
run_echo "echo \" \" \$USER \" \"" "echo \" \" \$USER \" \""
run_echo 'echo $a " " a' 'echo $a " " a'
run_echo 'echo "$ a "' 'echo "$ a "'
run_echo 'echo run_echo "" "" "" tomate' 'echo run_echo "" "" "" tomate'
run_echo 'echo $a run_echo $a "" "" "" "" tomate' 'echo $a run_echo "" "" "" "" tomate'
run_echo 'echo run_echo $a "" "" "" "" tomate' 'echo run_echo $a "" "" "" "" tomate'
run_echo 'echo run_echo $a "" "" "" "" $a tomate' 'echo run_echo $a "" "" "" "" $a tomate'
run_echo 'echo run_echo $a """""""" $a tomate' 'echo run_echo $a """""""" $a tomate'
run_echo 'echo ""$a""t' 'echo ""$a""t'
run_echo 'echo ""$a"" t' 'echo ""$a"" t'
run_echo 'echo ""$a" " t' 'echo ""$a" " t'
run_echo 'echo ""$a "  " t ""$a "  " t ' 'echo ""$a "  " t ""$a "  " t '
run_echo_redir '>kk echo run_echo' 'echo run_echo'
run_echo_redir '> kk echo run_echo' 'echo run_echo'
run_echo_redir '>kk echo -n run_echo' 'echo -n run_echo'
run_echo_redir '>kk echo -nnnnn -na run_echo' 'echo -nnnnn -na run_echo'
run_echo_redir '>kk echo -nnnnn' 'echo -nnnnn'
run_echo_redir '>kk echo' 'echo'


echo -e "\n"
echo "############"
echo "# EXIT     #"
echo "############"
echo -e "\n"

run_exit "exit" 0
run_exit "exit 42" 42
run_exit "exit 255" 255
run_exit "exit 255" 255
run_exit "exit 256" 0
run_exit "exit -1" 255
run_exit "exit -255" 1
run_exit "exit -256" 0
run_exit "exit 2147483647" 255
run_exit "exit 2147483648" 0
run_exit "exit 9223372036854775807" 255
run_exit "exit -9223372036854775808" 0
run_exit "exit 9223372036854775808" 2
run_exit "exit -9223372036854775809" 2
run_exit "exit -922337203685477580834" 2
run_exit "exit run_echo" 2

echo -e "\n"
echo "###################################"
echo "# VALOR RETORNO DE UN PROCESO     #"
echo "###################################"
echo -e "\n"

run_exit "/bin/run_echo" 127
run_command_return_value "/bin/ls" 0
run_command_return_value "/bin/run_echo" 127
run_command_return_value "/bin/run_echo" 127
run_command_return_value '/bin/grep "ejemplo" redirs/a' 0
run_command_return_value '/bin/grep "rollon" redirs/a' 1
run_command_return_value '/bin/grep "rollon" redirs/c' 2
run_command_return_value '/bin/diff redirs/a redirs/b' 1
run_command_return_value '/bin/diff redirs/a redirs/a' 0
run_command_return_value '/bin/diff redirs/a redirs/c' 2
run_command_return_value '/bin/test -f redirs/a' 0
run_command_return_value '/bin/test -f redirs/c' 1
run_command_return_value '/bin/test 5 -gt 3' 0
run_command_return_value '/bin/test 1 -gt 3' 1
run_command_return_value '/bin/test 3 -eq 3' 0
SHLVL_VAL=$SHLVL
run "expr $SHLVL_VAL + $SHLVL_VAL" "expr $SHLVL_VAL + $SHLVL_VAL"
run 'expr $SHLVL + $SHLVL' 'expr $SHLVL + $SHLVL - 2'

echo -e "\n"
echo "######################"
echo "# COMILLAS DOBLES    #"
echo "######################"
echo -e "\n"


run '"/bin/ls"' '"/bin/ls"'
run '"/bin/pwd"' '"/bin/pwd"'
run '"/bin/echo" run_echo' '"/bin/echo" run_echo'
run_echo2 '"/bin/echo" -n run_echo > temp1' '"/bin/echo" -n run_echo'
run_echo2 '"/bin/printf" run_echo > temp1' '"/bin/printf" run_echo'
run '"/bin/cat" redirs/a' '"/bin/cat" redirs/a'
run '"whoami"' '"whoami"'
run_command_invalid '""' '""'
run_command_invalid '   ""    ' '   ""    '
run '"/bin/ls" -la' '"/bin/ls" -la'
run '"/bin/ls" -l' '"/bin/ls" -l'
run '"echo" uno dos tres cuatro' '"echo" uno dos tres cuatro'
run '"/bin/ls" -l -a -h' '"/bin/ls" -l -a -h'
run '"/bin/ls" -lhS' '"/bin/ls" -lhS'
run '"/bin/cat" redirs/a redirs/b' '"/bin/cat" redirs/a redirs/b'
run '"head" -n 5 redirs/a' '"head" -n 5 redirs/a'
run '"""p"w"d"""' '"""p"w"d"""'
run_command_invalid '"pwd "' '"pwd "'
run_command_invalid '"run_echo"' '"run_echo"'
run 'echo "cat lol.c | cat > lol.c"' 'echo "cat lol.c | cat > lol.c"'
run_command_invalid '"ls "' '"ls "'
run_command_invalid '"abc def"' '"abc def"'
run_command_invalid '"echoo"' '"echoo"'
run_directory_invalid '"/noexiste"' '"/noexiste"'


echo -e "\n"
echo "#######################"
echo "# COMILLAS SIMPLES    #"
echo "#######################"
echo -e "\n"

run "'/bin/ls'" "'/bin/ls'"
run "'/bin/pwd'" "'/bin/pwd'"
run "'/bin/echo' run_echo" "'/bin/echo' run_echo"
run_echo2 "'/bin/echo' -n run_echo > temp1" "'/bin/echo' -n run_echo"
run "'/bin/cat' redirs/a" "'/bin/cat' redirs/a"
run "'whoami'" "'whoami'"
run_command_invalid "''" "''"
run_command_invalid "   ''    " "   ''    "
run "'/bin/ls' -la" "'/bin/ls' -la"
run "'/bin/ls' -l" "'/bin/ls' -l"
run "'echo' uno dos tres cuatro" "'echo' uno dos tres cuatro"
run "'/bin/ls' -l -a -h" "'/bin/ls' -l -a -h"
run "'/bin/ls' -lhS" "'/bin/ls' -lhS"
run "'/bin/cat' redirs/a redirs/b" "'/bin/cat' redirs/a redirs/b"
run "'head' -n 5 redirs/a" "'head' -n 5 redirs/a"
run "'''p'w'd'''" "'''p'w'd'''"
run_command_invalid "'pwd '" "'pwd '"
run_command_invalid "'run_echo'" "'run_echo'"
run "echo 'cat lol.c | cat > lol.c'" "echo 'cat lol.c | cat > lol.c'"
run_command_invalid "'ls '" "'ls '"
run_command_invalid "'abc def'" "'abc def'"
run_command_invalid "'echoo'" "'echoo'"
run_directory_invalid "'/noexiste'" "'/noexiste'"
run "'echo' '\$HOME'" "'echo' '\$HOME'"
run "'/bin/echo' '\$PATH'" "'/bin/echo' '\$PATH'"
run "'printenv' 'USER'" "'printenv' 'USER'"
run "'echo' '   uno   dos   tres   '" "'echo' '   uno   dos   tres   '"
run "'/bin/echo' '   '" "'/bin/echo' '   '"
run "'cat' '   redirs/a   '" "'cat' '   redirs/a   '"
run "'/bin/echo' run_echo '|' 'cat'" "'/bin/echo' run_echo '|' 'cat'"
run "'/bin/ls' '-l' '|' 'wc' '-l'" "'/bin/ls' '-l' '|' 'wc' '-l'"
run_directory_invalid "'cat' redirs/a '|' 'grep' hola" "'cat' redirs/a '|' 'grep' hola"
run "'echo' hola '>' redirs/salida.txt" "'echo' hola '>' redirs/salida.txt"
run "'cat' '<' redirs/a" "'cat' '<' redirs/a"
run_directory_invalid "'grep' hola '<' redirs/a '>' redirs/out" "'grep' hola '<' redirs/a '>' redirs/out"
run 'echo '\''$USER'\''' 'echo '\''$USER'\'''
run_directory_invalid 'cd '\''~'\''' 'cd '\''~'\'''
