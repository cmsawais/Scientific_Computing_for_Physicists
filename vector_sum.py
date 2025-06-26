#Vector Sum
a=3 # scalar
x= [1]*20 #vector of N=20 dimension
y= [4]* 20 #vector of N=20 dimension
z= [a*xi+yi for xi, yi in zip (x,y)]  #Calculates scalar product z=a*x+y 
with open ("task1.text", "w") as f:
f.write("z="+str(z)+"\n") #print values on text file


