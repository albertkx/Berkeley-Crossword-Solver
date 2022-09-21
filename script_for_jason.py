from tqdm import tqdm
import pickle
import numpy as np
import string
import sys
sys.path.append('DPR')
sys.path.append('DPR/transformers3')
from models import setup_closedbook, answer_clues
from datasets import load_dataset

def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))

def main():
    dataset = load_dataset('albertxu/CrosswordQA')
    dpr = setup_closedbook(0) 
    n_predictions = 10 # top-10 predictions for every question
    questions = dataset['validation']['clue']
    answers = dataset['validation']['answer']
    print(len(questions))
    uppercase_answers = []
    for answer in answers:
        if answer is None:
            uppercase_answers.append(None)
        else:
            uppercase_answers.append(''.join([x.upper() for x in answer if x.upper() in string.ascii_uppercase])) # convert to crossword style (e.g., ILOVEYOU)
    answers = uppercase_answers
    all_predictions = []
    correct = [0.0] * n_predictions
    total = [0.0] * n_predictions
    for batch_idx, item in tqdm(enumerate(chunker(list(zip(questions, answers)), 500))):
        questions, answers = list(zip(*item))
        all_words, all_scores = answer_clues(dpr, questions, 500, output_strings=True) # run model. We use 500 answers which we trim to just the top-10, if you increase this 500 the accuracy will go up
        for answers_idx, words in enumerate(all_words):
            answer = answers[answers_idx]
            scores = all_scores[answers_idx]
            if answer is None:
                continue
            answers_found = 0
            for idx, word in enumerate(words):
                words[idx] = ''.join([w.upper() for w in word if w.upper() in string.ascii_uppercase]) # convert to crossword style (e.g., ILOVEYOU)
                if len(words[idx]) != len(answer): # zero-out everything thats the wrong length
                    words[idx] = -1000
                    scores[idx] = -1000
                else:
                    answers_found += 1
                    if answers_found == n_predictions:
                        break
            words = [w for w in words if w != -1000][0:n_predictions]
            if len(words) != n_predictions: # just pad out the top-10 answers to make sure there are at least 10
                words.extend(['DUMMY ANSWER HERE'] * (n_predictions - len(words)))

            # get top-10 accuracy
            found = 0.0
            for index, word in enumerate(words):
                if word == answer:
                    found = 1.0
                correct[index] += found
                total[index] += 1.0
            all_predictions.append(words)
    print("Top-k Accuracies", [(cor/tot) for cor, tot in zip(correct, total)])
    with open('predictions.pkl','wb') as f:
        pickle.dump(all_predictions, f)

if __name__ == "__main__":
    main()
