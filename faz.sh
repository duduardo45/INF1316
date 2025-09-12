#!/bin/bash

# Este script compila e executa um arquivo C.
# Ele procura o arquivo .c no diretório atual e em subdiretórios.
# Uso: ./faz.sh nome_do_arquivo.c

# --- Validação do Input ---
# Verifica se o usuário forneceu um arquivo como argumento.
if [ -z "$1" ]; then
    echo "Erro: Você precisa especificar qual arquivo .c compilar."
    echo "Uso: $0 nome_do_arquivo.c"
    exit 1
fi

# Armazena o nome do arquivo C fornecido pelo usuário.
TARGET_FILENAME=$1

# --- Busca pelo Arquivo ---
echo "🔎 Procurando por '$TARGET_FILENAME'..."

# Procura pelo arquivo no diretório atual e em todos os subdiretórios.
# O resultado será o caminho completo, ex: ./lab3/para_sigkill.c
# Usamos mapfile para ler os resultados em um array, o que lida bem com nomes de arquivo complexos.
mapfile -t FOUND_FILES < <(find . -type f -name "$TARGET_FILENAME")
FILE_COUNT=${#FOUND_FILES[@]}

# Verifica a contagem de arquivos encontrados.
if [ "$FILE_COUNT" -eq 0 ]; then
    echo "❌ Erro: O arquivo '$TARGET_FILENAME' não foi encontrado no diretório atual ou em subdiretórios."
    exit 1
elif [ "$FILE_COUNT" -gt 1 ]; then
    echo "⚠️  Atenção: Mais de um arquivo com o nome '$TARGET_FILENAME' foi encontrado."
    echo "Por favor, seja mais específico. Arquivos encontrados:"
    # Imprime a lista de arquivos encontrados.
    printf '%s\n' "${FOUND_FILES[@]}"
    exit 1
fi

# Se chegamos aqui, exatamente um arquivo foi encontrado.
# O caminho do arquivo está no primeiro (e único) elemento do array.
SOURCE_FILE="${FOUND_FILES[0]}"
echo "✅ Arquivo encontrado: '$SOURCE_FILE'"


# --- Preparação para Compilação ---
# Define o nome do diretório de build.
BUILD_DIR="build"

# Cria o diretório de build se ele não existir. O '-p' evita erros caso o diretório já exista.
mkdir -p "$BUILD_DIR"

# Remove a extensão .c para obter o nome do executável (ex: para_sigkill.c -> para_sigkill).
EXEC_NAME="${TARGET_FILENAME%.c}"

# Monta o caminho completo para o executável que será criado.
EXEC_PATH="$BUILD_DIR/$EXEC_NAME"

# --- Compilação ---
echo "⚙️  Compilando '$SOURCE_FILE'..."

# Roda o compilador GCC.
# '-o' especifica o nome do arquivo de saída.
# As aspas duplas protegem contra nomes de arquivo com espaços.
gcc -o "$EXEC_PATH" "$SOURCE_FILE"

# --- Execução ---
# Verifica o código de saída do último comando (gcc). Se for 0, a compilação foi um sucesso.
if [ $? -eq 0 ]; then
    echo "✅ Compilação bem-sucedida!"
    echo "--------------------------------------------------"
    echo "🚀 Executando '$EXEC_PATH'..."
    echo ""
    # Executa o programa recém-criado.
    ./"$EXEC_PATH"
    echo ""
    echo "--------------------------------------------------"
    echo "✅ Execução finalizada."
else
    # Se o código de saída não for 0, algo deu errado.
    echo "❌ Erro na compilação. Verifique as mensagens do GCC acima."
    exit 1
fi

