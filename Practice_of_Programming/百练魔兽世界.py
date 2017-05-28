# Python3
# 题目来源：http://bailian.openjudge.cn/practice/3750/
# 测试数据参考：http://paste.ubuntu.com/12060696/

import re

class Warcraft(object):
	def __init__(self, belong, position, id, element, force, warcrafttype):
		self.__belong = belong
		self.__position = position
		self.__id = id
		self.__element = element
		self.__force = force
		self.__alive = True
		self.__warcrafttype = warcrafttype
	def moveto(self, newposition):
		self.__position = newposition
	def getwarcrafttype(self):
		return self.__warcrafttype
	def getid(self):
		return self.__id
	def getbelong(self):
		return self.__belong
	def getposition(self):
		return self.__position
	def getelement(self):
		return self.__element
	def getforce(self):
		return self.__force
	def isalive(self):
		return self.__alive
	def addelement(self, h):
		self.__element += h
	def setelement(self, h):
		self.__element = h
	def setforce(self, a):
		self.__force = a
	def double(self):
		self.__element *= 2
		self.__force *= 2
	def attack(self, defender):
		defender.__element -= self.__force
		if defender.__element <= 0: defender.__alive = False
	def fightagainst(self, attacker):
		attacker.__element -= self.__force//2
		if attacker.__element <= 0: attacker.__alive = False
	def prepare(self):
		pass
	def win(self, loser, attack):
		pass
	def lose(self, winner, attack):
		pass
		
class Dragon(Warcraft):
	def __init__(self, belong, position, id, element, force):
		Warcraft.__init__(self, belong, position, id, element, force, "dragon")
		
class Ninja(Warcraft):
	def __init__(self, belong, position, id, element, force):
		Warcraft.__init__(self, belong, position, id, element, force, "ninja")
	def fightagainst(self, attacker):
		pass

class Iceman(Warcraft):
	def __init__(self, belong, position, id, element, force):
		Warcraft.__init__(self, belong, position, id, element, force, "iceman")
		self.__stepcount = 0
	def moveto(self, newposition):
		Warcraft.moveto(self, newposition)
		self.__stepcount += 1
		if self.__stepcount % 2 == 0:
			self.addelement(-9)
			if self.getelement() <= 0: self.setelement(1)
			self.setforce(self.getforce()+20)

class Lion(Warcraft):
	def __init__(self, belong, position, id, element, force):
		Warcraft.__init__(self, belong, position, id, element, force, "lion")
		self.__elementold = element
	def prepare(self):
		self.__elementold = self.getelement()
	def lose(self, winner, attack):
		winner.addelement(self.__elementold)

class Wolf(Warcraft):
	def __init__(self, belong, position, id, element, force):
		Warcraft.__init__(self, belong, position, id, element, force, "wolf")
		self.__attackwin = 0
	def win(self, loser, attack):
		if attack:
			self.__attackwin += 1
			if self.__attackwin % 2 == 0:
				self.double()

class City(object):
	def __init__(self, position):
		self.__position = position
		self.__element = 0
		self.__redwarcraft = None
		self.__bluewarcraft = None
		self.__flag = None
		self.__winnerlog = [None, None]
	def produce(self, addelement = 10):
		self.__element += addelement
	def restat(self):
		self.__redwarcraft = None
		self.__bluewarcraft = None
	def setredwarcraft(self, warcraft):
		self.__redwarcraft = warcraft
	def setbluewarcraft(self, warcraft):
		self.__bluewarcraft = warcraft
	def getredwarcraft(self):
		return self.__redwarcraft
	def getbluewarcraft(self):
		return self.__bluewarcraft
	def getelement(self):
		return self.__element
	def resetelement(self):
		self.__element = 0
	def winnerlog(self, result):
		self.__winnerlog[0], self.__winnerlog[1] = self.__winnerlog[1], result
		return self.__winnerlog[0] == self.__winnerlog[1]
	def getflag(self):
		return self.__flag
	def setflag(self, flag):
		self.__flag = flag
				
class HeadQuarter(object):
	def __init__(self, position, headquarterelementinit, warcraftelementinit, warcraftforceinit, belong, order):
		self.__position = position
		self.__index = 0
		self.__element = headquarterelementinit
		self.__warcraftelementinit = warcraftelementinit
		self.__warcraftforceinit = warcraftforceinit
		self.__belong = belong
		self.__order = order
	def born(self, warcraftid):
		warcrafttype = self.__order[self.__index]
		if self.__element >= self.__warcraftelementinit[warcrafttype]:
			if warcrafttype == "dragon": warcraft = Dragon(self.__belong, self.__position, warcraftid, self.__warcraftelementinit[warcrafttype], self.__warcraftforceinit[warcrafttype])
			elif warcrafttype == "ninja": warcraft = Ninja(self.__belong, self.__position, warcraftid, self.__warcraftelementinit[warcrafttype], self.__warcraftforceinit[warcrafttype])
			elif warcrafttype == "iceman": warcraft = Iceman(self.__belong, self.__position, warcraftid, self.__warcraftelementinit[warcrafttype], self.__warcraftforceinit[warcrafttype])
			elif warcrafttype == "lion": warcraft = Lion(self.__belong, self.__position, warcraftid, self.__warcraftelementinit[warcrafttype], self.__warcraftforceinit[warcrafttype])
			else: warcraft = Wolf(self.__belong, self.__position, warcraftid, self.__warcraftelementinit[warcrafttype], self.__warcraftforceinit[warcrafttype])
			self.__element -= self.__warcraftelementinit[warcrafttype]
			self.__index = (self.__index+1) % 5
			return warcraft
		else:
			return None
	def addelement(self, city):
		self.__element += city.getelement()
		city.resetelement()
	def sendelement(self, warcraft, element = 8):
		if self.__element >= element:
			warcraft.addelement(element)
			self.__element -= element
	def getelement(self):
		return self.__element
			
class RedHeadQuarter(HeadQuarter):
	def __init__(self, position, headquarterelementinit, warcraftelementinit, warcraftforceinit):
		HeadQuarter.__init__(self, position, headquarterelementinit, warcraftelementinit, warcraftforceinit, "red", ["iceman", "lion", "wolf", "ninja", "dragon"])

class BlueHeadQuarter(HeadQuarter):
	def __init__(self, position, headquarterelementinit, warcraftelementinit, warcraftforceinit):
		HeadQuarter.__init__(self, position, headquarterelementinit, warcraftelementinit, warcraftforceinit, "blue", ["lion", "dragon", "ninja", "iceman", "wolf"])
		
class Timer(object):
	def __init__(self, maxtime):
		self.__time = 0
		self.__maxtime = maxtime
	def gettimestr(self):
		return str(self.__time//60).zfill(3)+":"+str(self.__time%60).zfill(2)
	def gettime(self):
		return self.__time
	def tick(self):
		self.__time += 10
		if self.__time > self.__maxtime: return False
		else: return True
		
class World(object):
	def __init__(self, worldparastr, warcraftelementstr, warcraftforcestr):
		worldpara = [int(j) for j in re.subn(' +', ' ', worldparastr)[0].split(' ')]
		headquarterelementinit = worldpara[0]
		citynum = worldpara[1]
		maxtime = worldpara[2]
		warcraftelement = [int(j) for j in re.subn(' +', ' ', warcraftelementstr)[0].split(' ')]
		warcraftelementinit = {"dragon": warcraftelement[0], "ninja": warcraftelement[1], "iceman": warcraftelement[2], "lion": warcraftelement[3], "wolf": warcraftelement[4]}
		warcraftforce = [int(j) for j in re.subn(' +', ' ', warcraftforcestr)[0].split(' ')]
		warcraftforceinit = {"dragon": warcraftforce[0], "ninja": warcraftforce[1], "iceman": warcraftforce[2], "lion": warcraftforce[3], "wolf": warcraftforce[4]}		
		self.__timer = Timer(maxtime)		
		self.__redheadquarter = RedHeadQuarter(0, headquarterelementinit, warcraftelementinit, warcraftforceinit)
		self.__blueheadquarter = BlueHeadQuarter(citynum+1, headquarterelementinit, warcraftelementinit, warcraftforceinit)
		self.__headquarter = {"red": self.__redheadquarter, "blue": self.__blueheadquarter}		
		self.__city = {}
		for i in range(citynum):
			self.__city[i+1] = City(i+1)		
		self.__redwarcraftlst = []
		self.__bluewarcraftlst = []
		
	def __born(self):
		rednewwarcraft = self.__redheadquarter.born(len(self.__redwarcraftlst)+1)
		if rednewwarcraft != None:
			self.__redwarcraftlst.append(rednewwarcraft)
			print ("%s red %s %d born" % (self.__timer.gettimestr(), rednewwarcraft.getwarcrafttype(), rednewwarcraft.getid()))
		bluenewwarcraft = self.__blueheadquarter.born(len(self.__bluewarcraftlst)+1)
		if bluenewwarcraft != None:
			self.__bluewarcraftlst.append(bluenewwarcraft)
			print ("%s blue %s %d born" % (self.__timer.gettimestr(), bluenewwarcraft.getwarcrafttype(), bluenewwarcraft.getid()))
	
	def __move(self):
		outputstr = {}
		warcraftinblueheadquarter = 0
		warcraftinredheadquarter = 0
		for w in self.__redwarcraftlst:
			if w.isalive() and w.getposition() <= len(self.__city):
				w.moveto(w.getposition()+1)
				if w.getposition() == len(self.__city)+1:
					outputstr[str(len(self.__city)+1)+"red"] = "%s red %s %d reached blue headquarter with %d elements and force %d" % (self.__timer.gettimestr(), w.getwarcrafttype(), w.getid(), w.getelement(), w.getforce())
				else:
					outputstr[str(w.getposition())+"red"] = "%s red %s %d marched to city %d with %d elements and force %d" % (self.__timer.gettimestr(), w.getwarcrafttype(), w.getid(), w.getposition(), w.getelement(), w.getforce())
			if w.isalive() and w.getposition() == len(self.__city)+1:
				warcraftinblueheadquarter += 1
		for w in self.__bluewarcraftlst:
			if w.isalive() and w.getposition() > 0:
				w.moveto(w.getposition()-1)
				if w.getposition() == 0:
					outputstr["0blue"] = "%s blue %s %d reached red headquarter with %d elements and force %d" % (self.__timer.gettimestr(), w.getwarcrafttype(), w.getid(), w.getelement(), w.getforce())
				else:
					outputstr[str(w.getposition())+"blue"] = "%s blue %s %d marched to city %d with %d elements and force %d" % (self.__timer.gettimestr(), w.getwarcrafttype(), w.getid(), w.getposition(), w.getelement(), w.getforce())
			if w.isalive() and w.getposition() == 0:
				warcraftinredheadquarter += 1
		if warcraftinblueheadquarter >= 2:
			outputstr[str(len(self.__city)+1)+"red"] += "\n%s blue headquarter was taken" % (self.__timer.gettimestr())
		if warcraftinredheadquarter >= 2:
			outputstr["0blue"] += "\n%s red headquarter was taken" % (self.__timer.gettimestr())
		for i in range(0, len(self.__city)+2):
			for j in ["red", "blue"]:
				if str(i)+j in outputstr:
					print (outputstr[str(i)+j])
		return warcraftinblueheadquarter >= 2 or warcraftinredheadquarter >= 2
					
	def __produce(self):
		for c in self.__city.values():
			c.produce(10)
			
	def __citystat(self):
		for c in self.__city.values():
			c.restat()
		for w in self.__redwarcraftlst:
			if w.isalive() and w.getposition() >= 1 and w.getposition() <= len(self.__city):
				self.__city[w.getposition()].setredwarcraft(w)
		for w in self.__bluewarcraftlst:
			if w.isalive() and w.getposition() >= 1 and w.getposition() <= len(self.__city):
				self.__city[w.getposition()].setbluewarcraft(w)
				
	def __earn(self):
		for i in range(1, len(self.__city)+1):
			if self.__city[i].getredwarcraft() != None and self.__city[i].getbluewarcraft() == None:
				print ("%s red %s %d earned %d elements for his headquarter" % (self.__timer.gettimestr(), self.__city[i].getredwarcraft().getwarcrafttype(), self.__city[i].getredwarcraft().getid(), self.__city[i].getelement()))
				self.__redheadquarter.addelement(self.__city[i])
			elif self.__city[i].getbluewarcraft() != None and self.__city[i].getredwarcraft() == None:
				print ("%s blue %s %d earned %d elements for his headquarter" % (self.__timer.gettimestr(), self.__city[i].getbluewarcraft().getwarcrafttype(), self.__city[i].getbluewarcraft().getid(), self.__city[i].getelement()))
				self.__blueheadquarter.addelement(self.__city[i])
				
	def __battle(self):
		winnerlst = {"red": [], "blue": []}
		wincitylst = {"red": [], "blue": []}
		for i in range(1, len(self.__city)+1):
			if self.__city[i].getredwarcraft() != None and self.__city[i].getbluewarcraft() != None:
				attackfirst = self.__city[i].getflag()
				if attackfirst == None:
					if i % 2 == 1: attackfirst = "red"
					else: attackfirst = "blue"
				if attackfirst == "red":
					attacker = self.__city[i].getredwarcraft()
					defender = self.__city[i].getbluewarcraft()
				else:
					attacker = self.__city[i].getbluewarcraft()
					defender = self.__city[i].getredwarcraft()
				print ("%s %s %s %d attacked %s %s %d in city %d with %d elements and force %d" % (self.__timer.gettimestr(), attacker.getbelong(), attacker.getwarcrafttype(), attacker.getid(), defender.getbelong(), defender.getwarcrafttype(), defender.getid(), i, attacker.getelement(), attacker.getforce()))
				attacker.prepare()
				defender.prepare()
				attacker.attack(defender)
				winner = None
				loser = None
				if defender.isalive():
					if defender.getwarcrafttype() != "ninja": print ("%s %s %s %d fought back against %s %s %d in city %d" % (self.__timer.gettimestr(), defender.getbelong(), defender.getwarcrafttype(), defender.getid(), attacker.getbelong(), attacker.getwarcrafttype(), attacker.getid(), i))
					defender.fightagainst(attacker)
					if not attacker.isalive():
						defender.win(attacker, False)
						attacker.lose(defender, False)
						winner, loser = defender, attacker
				else:
					attacker.win(defender, True)
					defender.lose(attacker, True)
					winner, loser = attacker, defender
				if winner != None and loser != None:
					print ("%s %s %s %d was killed in city %d" % (self.__timer.gettimestr(), loser.getbelong(), loser.getwarcrafttype(), loser.getid(), i))
				if attacker.isalive() and attacker.getwarcrafttype() == "dragon":
					print ("%s %s dragon %d yelled in city %d" % (self.__timer.gettimestr(), attacker.getbelong(), attacker.getid(), i))
				if winner != None and loser != None:
					winnerlst[winner.getbelong()].append(winner)
					wincitylst[winner.getbelong()].append(self.__city[i])
					print ("%s %s %s %d earned %d elements for his headquarter" % (self.__timer.gettimestr(), winner.getbelong(), winner.getwarcrafttype(), winner.getid(), self.__city[i].getelement()))
					if self.__city[i].winnerlog(winner.getbelong()) and self.__city[i].getflag() != winner.getbelong():
						print ("%s %s flag raised in city %d" % (self.__timer.gettimestr(), winner.getbelong(), i))
						self.__city[i].setflag(winner.getbelong())
				else:
					self.__city[i].winnerlog(None)
		winnerlst["red"].reverse()
		for lst in winnerlst.values():
			for winner in lst:
				self.__headquarter[winner.getbelong()].sendelement(winner, 8)
		for belong, lst in wincitylst.items():
			for city in lst:
				self.__headquarter[belong].addelement(city)
					
	def __report(self):
		print ("%s %d elements in red headquarter" % (self.__timer.gettimestr(), self.__redheadquarter.getelement()))
		print ("%s %d elements in blue headquarter" % (self.__timer.gettimestr(), self.__blueheadquarter.getelement()))
	
	def run(self):
		while True:
			time = self.__timer.gettime()
			if time % 60 == 0:
				self.__born()
			elif time % 60 == 10:
				if self.__move(): break
			elif time % 60 == 20:
				self.__produce()
			elif time % 60 == 30:
				self.__citystat()
				self.__earn()
			elif time % 60 == 40:
				# self.__citystat()
				self.__battle()
			elif time % 60 == 50:
				self.__report()
			if not self.__timer.tick():
				break
		
if __name__ == '__main__':
	casenum = int(input())
	for i in range(1, casenum+1):
		print ("Case:%d" % (i))
		world = World(input(), input(), input())
		world.run()
