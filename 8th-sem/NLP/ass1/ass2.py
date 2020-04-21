from nltk.corpus import stopwords 
from nltk.tokenize import word_tokenize 
from nltk.stem import WordNetLemmatizer 
from nltk.stem import PorterStemmer

lemmatizer = WordNetLemmatizer()
ps = PorterStemmer()
example_sent = "Speech recognition is the first step to build computers like us"

stop_words = set(stopwords.words('english')) 

word_tokens = word_tokenize(example_sent) 

filtered_sentence = [w for w in word_tokens if not w in stop_words] 

filtered_sentence = [] 

for w in word_tokens: 
	if w not in stop_words: 
		filtered_sentence.append(lemmatizer.lemmatize(w)) 

print(word_tokens) 
print(filtered_sentence) 
