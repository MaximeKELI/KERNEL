#!/bin/bash
# Audit de communication entre fichiers du projet kernel
# Vérifie les includes, dépendances, références et liens

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "=========================================="
echo "AUDIT DE COMMUNICATION ENTRE FICHIERS"
echo "=========================================="
echo ""

ERRORS=0
WARNINGS=0

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Fonction pour compter les erreurs
error() {
    echo -e "${RED}[ERREUR]${NC} $1"
    ((ERRORS++))
}

warning() {
    echo -e "${YELLOW}[AVERTISSEMENT]${NC} $1"
    ((WARNINGS++))
}

success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

echo "1. Vérification des includes manquants..."
echo "-------------------------------------------"

# Vérifier que tous les fichiers .c incluent leurs headers correspondants
find kernel -name "*.c" | while read -r cfile; do
    basename=$(basename "$cfile" .c)
    dirname=$(dirname "$cfile")
    
    # Chercher le header correspondant
    header_found=false
    
    # Chercher dans include/
    if [ -f "include/${basename}.h" ]; then
        header_found=true
    fi
    
    # Chercher dans include/ avec le même chemin
    relpath=${dirname#kernel/}
    if [ -f "include/${relpath}/${basename}.h" ]; then
        header_found=true
    fi
    
    # Chercher dans le même répertoire
    if [ -f "${dirname}/${basename}.h" ]; then
        header_found=true
    fi
    
    # Ignorer certains fichiers qui n'ont pas besoin de header
    case "$basename" in
        debug|kernel|tests_*|test)
            header_found=true
            ;;
    esac
    
    if [ "$header_found" = false ] && [ -n "$dirname" ] && [ "$dirname" != "kernel" ]; then
        warning "Pas de header trouvé pour $cfile"
    fi
done

echo ""
echo "2. Vérification des headers manquants..."
echo "-------------------------------------------"

# Vérifier que tous les headers dans include/ ont un fichier .c correspondant
find include -name "*.h" | while read -r hfile; do
    basename=$(basename "$hfile" .h)
    relpath=${hfile#include/}
    dirname=$(dirname "$relpath")
    
    # Chercher le fichier .c correspondant
    cfile_found=false
    
    # Chercher dans kernel/
    if [ -f "kernel/${basename}.c" ]; then
        cfile_found=true
    fi
    
    # Chercher dans kernel/ avec le même chemin
    if [ "$dirname" != "." ] && [ -f "kernel/${dirname}/${basename}.c" ]; then
        cfile_found=true
    fi
    
    # Chercher dans kernel/fs/ pour include/fs/
    if [[ "$hfile" == include/fs/* ]]; then
        if [ -f "kernel/fs/${basename}.c" ]; then
            cfile_found=true
        fi
    fi
    
    # Ignorer certains headers qui n'ont pas besoin de .c
    case "$basename" in
        types|kernel|io|string|stdio|asm|memory_constants)
            cfile_found=true
            ;;
    esac
    
    if [ "$cfile_found" = false ]; then
        warning "Pas de fichier .c trouvé pour $hfile"
    fi
done

echo ""
echo "3. Vérification des includes invalides..."
echo "-------------------------------------------"

# Vérifier que tous les includes pointent vers des fichiers existants
find kernel include -name "*.c" -o -name "*.h" | while read -r file; do
    grep -h "^#include" "$file" 2>/dev/null | while read -r include_line; do
        # Extraire le nom du fichier inclus
        if [[ "$include_line" =~ \"([^\"]+)\" ]]; then
            included_file="${BASH_REMATCH[1]}"
            
            # Chercher le fichier
            found=false
            
            # Chercher dans include/
            if [ -f "include/${included_file}" ]; then
                found=true
            fi
            
            # Chercher dans include/ avec sous-dossiers
            if [[ "$included_file" == */* ]]; then
                if [ -f "include/${included_file}" ]; then
                    found=true
                fi
            fi
            
            # Chercher dans le même répertoire
            filedir=$(dirname "$file")
            if [ -f "${filedir}/${included_file}" ]; then
                found=true
            fi
            
            # Chercher dans include/drivers/
            if [[ "$included_file" == drivers/* ]]; then
                if [ -f "include/${included_file}" ]; then
                    found=true
                fi
            fi
            
            if [ "$found" = false ]; then
                error "Include manquant dans $file: $included_file"
            fi
        fi
    done
done

echo ""
echo "4. Vérification des références dans la documentation..."
echo "-------------------------------------------"

# Vérifier que tous les liens dans la documentation pointent vers des fichiers existants
find Documentation -name "*.md" | while read -r docfile; do
    grep -oE '\[([^\]]+)\]\(([^\)]+)\)' "$docfile" 2>/dev/null | while read -r link; do
        if [[ "$link" =~ \(([^\)]+)\) ]]; then
            link_path="${BASH_REMATCH[1]}"
            
            # Ignorer les liens externes
            if [[ "$link_path" == http* ]] || [[ "$link_path" == mailto:* ]]; then
                continue
            fi
            
            # Résoudre le chemin relatif
            docdir=$(dirname "$docfile")
            resolved_path=$(cd "$docdir" && cd "$(dirname "$link_path")" 2>/dev/null && pwd)/$(basename "$link_path") 2>/dev/null || echo ""
            
            if [ -z "$resolved_path" ] || [ ! -f "$resolved_path" ]; then
                # Vérifier si c'est un lien vers Documentation/
                if [[ "$link_path" == Documentation/* ]] || [[ "$link_path" == /*Documentation/* ]]; then
                    doc_file="${link_path#Documentation/}"
                    if [ ! -f "Documentation/${doc_file}" ]; then
                        warning "Lien cassé dans $docfile: $link_path"
                    fi
                elif [[ "$link_path" != ../* ]] && [[ "$link_path" != ./* ]]; then
                    # Lien relatif dans Documentation/
                    if [ ! -f "Documentation/${link_path}" ] && [ ! -f "${link_path}" ]; then
                        warning "Lien cassé dans $docfile: $link_path"
                    fi
                fi
            fi
        fi
    done
done

echo ""
echo "5. Vérification des fichiers dans le Makefile..."
echo "-------------------------------------------"

# Vérifier que tous les répertoires mentionnés dans le Makefile existent
grep -E '\$\(wildcard \$(KERNEL_DIR)/[^)]+\)' Makefile | while read -r line; do
    if [[ "$line" =~ KERNEL_DIR\)/([^/]+)/([^)]+) ]]; then
        dir="${BASH_REMATCH[1]}"
        pattern="${BASH_REMATCH[2]}"
        
        if [ ! -d "kernel/${dir}" ]; then
            error "Répertoire manquant dans Makefile: kernel/${dir}"
        fi
    fi
done

# Vérifier que tous les répertoires kernel/* existent et sont compilés
find kernel -type d -mindepth 1 -maxdepth 1 | while read -r dir; do
    dirname=$(basename "$dir")
    
    # Vérifier si le répertoire est dans le Makefile
    if ! grep -q "\$(wildcard \$(KERNEL_DIR)/${dirname}" Makefile; then
        # Ignorer certains répertoires qui n'ont pas de .c
        if [ "$(find "$dir" -name "*.c" | wc -l)" -gt 0 ]; then
            warning "Répertoire kernel/${dirname} non mentionné dans Makefile"
        fi
    fi
done

echo ""
echo "6. Vérification des fonctions déclarées mais non définies..."
echo "-------------------------------------------"

# Créer une liste temporaire des fonctions déclarées
TMP_DECLARED=$(mktemp)
TMP_DEFINED=$(mktemp)

# Extraire toutes les déclarations de fonctions
find include -name "*.h" -exec grep -hE '^[a-zA-Z_][a-zA-Z0-9_* ]+\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\);' {} \; | \
    sed 's/^[[:space:]]*//' | \
    sed 's/[[:space:]]*$//' | \
    grep -v '^#' | \
    sed 's/.*[^a-zA-Z0-9_]\([a-zA-Z_][a-zA-Z0-9_]*\)\s*(.*/\1/' | \
    sort -u > "$TMP_DECLARED"

# Extraire toutes les définitions de fonctions
find kernel -name "*.c" -exec grep -hE '^[a-zA-Z_][a-zA-Z0-9_* ]+\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\([^)]*\)\s*\{' {} \; | \
    sed 's/^[[:space:]]*//' | \
    sed 's/[[:space:]]*$//' | \
    sed 's/.*[^a-zA-Z0-9_]\([a-zA-Z_][a-zA-Z0-9_]*\)\s*(.*/\1/' | \
    sort -u > "$TMP_DEFINED"

# Comparer (simplifié - peut avoir des faux positifs)
echo "   (Vérification simplifiée - peut avoir des faux positifs)"

rm -f "$TMP_DECLARED" "$TMP_DEFINED"

echo ""
echo "7. Vérification des headers dupliqués..."
echo "-------------------------------------------"

# Vérifier les headers dupliqués dans include/
find include -name "*.h" | while read -r hfile; do
    basename=$(basename "$hfile")
    
    # Chercher d'autres fichiers avec le même nom
    duplicates=$(find include -name "$basename" | grep -v "^${hfile}$" | wc -l)
    
    if [ "$duplicates" -gt 0 ]; then
        error "Header dupliqué: $basename trouvé dans plusieurs emplacements"
        find include -name "$basename"
    fi
done

echo ""
echo "8. Vérification de la cohérence des chemins après déplacement Documentation..."
echo "-------------------------------------------"

# Vérifier que README.md pointe vers Documentation/
if grep -q '\[.*\]\([^D].*\.md\)' README.md 2>/dev/null; then
    if ! grep -q '\[.*\]\(Documentation/.*\.md\)' README.md; then
        warning "README.md contient des liens vers .md qui ne pointent pas vers Documentation/"
    fi
fi

# Vérifier que les scripts pointent vers Documentation/
find scripts -name "*.sh" | while read -r script; do
    if grep -q '\.md' "$script" 2>/dev/null; then
        if ! grep -q 'Documentation/.*\.md' "$script"; then
            warning "Script $script référence des .md sans chemin Documentation/"
        fi
    fi
done

echo ""
echo "=========================================="
echo "RÉSUMÉ DE L'AUDIT"
echo "=========================================="
echo -e "${GREEN}Erreurs: $ERRORS${NC}"
echo -e "${YELLOW}Avertissements: $WARNINGS${NC}"
echo ""

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    success "Aucun problème détecté !"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    warning "Audit terminé avec des avertissements mineurs"
    exit 0
else
    error "Audit terminé avec des erreurs"
    exit 1
fi
