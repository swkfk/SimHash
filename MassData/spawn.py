import os
import random

from bs4 import BeautifulSoup
from faker import Faker
from tqdm import tqdm

def article_content(filename):
    """
        Read texts from the html file.

    @para :filename: the `html` file in the `source` folder
    @return the content in `str` type
    """
    html = ""
    with open(f"./source/{filename}", "r") as f:
        html = f.read()
    soup = BeautifulSoup(html, "html.parser")

    for script in soup(["script","style"]):
        script.extract()

    texts = "\n".join(line.strip() for line in soup.get_text().splitlines() if line)

    return ''.join([i if ord(i) < 128 else ' ' for i in texts])

if __name__ == "__main__":
    fake = Faker()

    # Articles
    with open("article.txt", "w") as f:
        idx = 0
        for file in tqdm(os.listdir("./source") * 2):
            text = article_content(file)
            f.write(f"1-{fake.ssn()}-{idx}\n")
            f.write(text)
            f.write("\n\f\n")
            idx += 1

    # Samples
    with open("sample.txt", "w") as f:
        idx = 0
        for file in tqdm(os.listdir("./source")[::80]):
            text = article_content(file)
            f.write(f"S-{fake.ssn()}-{idx}\n")
            f.write(text)
            f.write("\n\f\n")
            idx += 1

