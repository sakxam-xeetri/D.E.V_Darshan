import re

with open("d:/D.E.V_Darshan/full_exam_solutions.txt", "r", encoding="utf-8") as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    stripped = line.strip()
    if len(stripped) > 0 and (set(stripped) == {'='} or set(stripped) == {'-'}):
        new_lines.append((stripped[0] * 32) + '\n')
    else:
        new_lines.append(line)

text = ''.join(new_lines)

with open("d:/D.E.V_Darshan/full_exam_solutions.txt", "w", encoding="utf-8") as f:
    f.write(text)

print("All divider bars set strictly to 32 characters!")
