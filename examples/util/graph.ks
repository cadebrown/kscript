#!/usr/bin/env ks
import util

# Create from a list of integers
x = util.Graph({
    "a": {
        "b": 1,
        "c": 2
    },
    "b": {
        "d": 3
    },
    "c": {
        "d": 4
    },
    "d": {
    }
})

print (x._dotfile())


for i in x.dfs() {
    print (i)
}


bst = util.BST()
bst[3] = "c"
bst[1] = "a"
bst[2] = "b"

print (bst)

