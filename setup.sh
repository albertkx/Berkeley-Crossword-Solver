# Install Dependencies
pip install scipy
mv DPR/transformers3 ./
pip install -e DPR/
mv transformers3 DPR/
pip install -e DPR/transformers3/
pip install transformers

# Download checkpoints
mkdir -p checkpoints/biencoder/embeddings
wget https://huggingface.co/albertxu/Berkeley-Crossword-Solver/resolve/main/biencoder/dpr_biencoder.bin -O checkpoints/biencoder/dpr_biencoder.bin
wget https://huggingface.co/albertxu/Berkeley-Crossword-Solver/resolve/main/biencoder/wordlist.tsv -O checkpoints/biencoder/wordlist.tsv
# Download precomputed embeddings
for i in {0..3}; do
    wget https://huggingface.co/albertxu/Berkeley-Crossword-Solver/resolve/main/biencoder/embeddings/embeddings.json_$i.pkl -O checkpoints/biencoder/embeddings/embeddings.json_$i.pkl
done
