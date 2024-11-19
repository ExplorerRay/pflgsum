import re
import random

def deidentify(file):
    # read every line of file (test_cases/maillog)
    # deidentify the file

    # regex match the fields
    email_addr = re.compile(r"([\w\-\.]+)@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])")
    domain = re.compile(r"((?!-)[A-Za-z0-9-]{1,63}(?<!-)\.)+[A-Za-z]{2,6}")
    ip = re.compile(r"\b(?:[0-9]{1,3}\.){3}([0-9]{1,3})\b")
    username = re.compile(r"username=([\w-]+)")

    output = open('test_cases/de_maillog', 'w')
    with open(file, 'r') as f:
        data = f.readlines()

        for line in data:
            if re.search(username, line):
                rep = lambda x: x.group(0).split('=')[0] + '=' + len(x.group(0).split('=')[1]) * 'n'
                line = re.sub(username, rep, line)
            if re.search(ip, line):
                # hash orginal ip and mod for group 1 to 4
                rep = lambda x: [str(hash(s) % 256) for s in x.group(0).split('.')]
                line = re.sub(ip, lambda x: '.'.join(rep(x)), line)
            if re.search(email_addr, line):
                rep = lambda x: len(x.group(0).split('@')[0]) * 'u' + '@' + x.group(0).split('@')[1]
                line = re.sub(email_addr, rep, line)

            if re.search(domain, line):
                dot = random.randint(1, 3)
                rep = ''
                r = random.randint(0, 1)
                for i in range(dot+1):
                    if r == 0:
                        rep += (i + 1) * 'x'
                    else:
                        rep += (dot - i + 1) * 'x'
                    if i != dot:
                        rep += '.'
                line = re.sub(domain, rep, line)
            output.write(line)
    output.close()

deidentify('maillog')
