#!/usr/bin/env ks
""" graphs.ks - demonstrates how to work with the built in 'graph' data type


"""

"\N[asdf]"



# create example of TSP
x = graph([
    "A",
    "B",
    "C",
    "D",
], [
    (0, 1, 10.0),
    (0, 2, 15.0),
    (0, 3, 20.0),
    (1, 2, 35.0),
    (1, 3, 25.0),
    (2, 3, 30.0),
])

# make symmetric
new_edges = []
for (i, j, val) in x.edges {
    if i < j {
        x.edges.push(j, i, val)
    }
}


#print (x)

x = y * 3

func x(a, b=3, *c) {
    ret func (x) {
        ret x
    }
}
func y {

}
type Z extends Y {
    
}
x = `x+y7`


type f extends f f {

}
type X {
    
}


