git add "Comma\account"
git add "Comma\admin"
git add "Comma\agreement"
git add "Comma\assets"
git add "Comma\call"
git add "Comma\index"
git add "Comma\moderation"
git add "Comma\policy"
git add "Comma\profile"
git add "Comma\undefined"
git add "Comma\config.txt"
git add "Comma\json.hpp"
git add "Comma\favicon.ico"
git add "Comma\main.cpp"
git add "Comma\robots.txt"
git add "Comma\server.crt"
git add "Comma\server.key"
git add "Comma\sqlite3.c"
git add "Setup\Debug"
git add "push.bat"
git commit -m "upd"
git pull origin main --rebase
git push origin main
gh release delete Release --yes 2>nul
git push --delete origin Release 2>nul
git tag -d Release 2>nul
gh release create Release "Setup\Debug\Setup.msi" --title "Release" --notes ""