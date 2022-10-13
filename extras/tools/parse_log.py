f = open("cook.log")
lines = f.readlines()
f.close()

severity_to_watch = ["Warning", "Error"]
keywords_to_watch = ["is not initialized properly"]

problems = []


for each_line in lines:
    each_line = each_line.strip().replace("\n", "")
    for each_keyword in keywords_to_watch:
        if each_keyword in each_line:
            problems.append(each_line)


    line = each_line.split(":")

    try:
        log_cat = line[0].strip()
        severity = line[1].strip()
    except:
        continue
    
    if severity in severity_to_watch:
        problems.append(each_line)


for each_problem in problems:
    print(each_problem)

