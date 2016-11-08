import sys
import random

if len(sys.argv) != 4:
	print("not enough command line arguments")
	print("usage: tour <numSites> <numDays> <probabilityOfSiteBeingOpenOnAnyDay>")
	exit(0)

numSites = int(sys.argv[1])
probabilityOfBeingOpen = float(sys.argv[3])
numDays = int(sys.argv[2])

def generateSiteInfo(n):
	openHours = []
	for i in range(0,numDays):
		if random.random() < probabilityOfBeingOpen:
			openHours.append((n,i+1,random.randint(6,11),random.randint(15,22)))
	if len(openHours) > 0:
		minOpenInterval = min(openHours,key=lambda tup:tup[3]-tup[2])
		minOpenDuration = minOpenInterval[3] - minOpenInterval[2]
	else:
		minOpenDuration = 30
	
	avenue = random.randint(1,200)
	street = random.randint(1,200)
	desiredTime = random.randint(30,max(minOpenDuration,240))
	value = random.random()*180+20
	siteInfo = [(n,avenue,street,desiredTime,value)] + openHours
	siteInfo = [" ".join([str(i) for i in info]) for info in siteInfo]
	return siteInfo

siteStrings = []
timeStrings = []
for i in range(0,numSites):
	siteInfo = generateSiteInfo(i)
	siteStrings.append(siteInfo[0])
	if len(siteInfo) > 1:
		timeStrings = timeStrings + siteInfo[1:]

# print the file to standard output for piping/redirection

print("site avenue street desiredtime value")
for s in siteStrings:
	print(s)
print("    ")
print("site day beginhour endhour")
for i in range(0,len(timeStrings)):
	if i == len(timeStrings)-1:
		print(timeStrings[i]),
	else:
		print(timeStrings[i])

