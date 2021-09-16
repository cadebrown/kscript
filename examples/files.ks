#!/usr/bin/env ks
""" examples/files.ks - examples of how to use basic file operations in kscript


@author: Cade Brown <cade@kscript.org>
"""

# To open a file in kscript, use the global builtin function 'open':
fp = open('README.md')

# Optionally, you could have:
# fp = open('README.md', 'rb')
# Which uses the second (optional) parameter, 'mode', describing the text mode of the file
#   you're trying to open

# Now, you can read the entire contents with '.read()'
src = fp.read()

# Print out the length
print('len: {len(src)}')

# You could also specify a maximum number of characters with 'fp.read(maxsize)'

# If you 'open' a file, you must remember to call '.close()' when you are done with it!
# Unless you use a 'with' block...
fp.close()


# Instead of having to manually call '.close', kscript has a way to keep code tidier when
#   the usage pattern is linear:
# Use 'with' when you will only be using the object inside the code blocks (because it
#   will be automatically released afterwards)
with fp = open('README.md') {

    # You could run the same code as before, but that's boring!
    # kscript also provides ways to iterate over files more efficiently:
    for line in fp.lines() {
        # Now, you have each line (without the '\n' at the end) in the file
        print ('first line: {len(line)}')
        # Exit out of this for loop, so we don't empty the file
        break
    }

    # Now, we start reading where we left off, but this time we use a different splitting technique
    # We use a regex (indicated with backticks), which is NOT included in the resulting words
    # 'fp.lines()' is the same as 'fp.split(`\r?\n`)'
    for i, word in enumerate(fp.split(`\s+`)) {
        print('next word #{i}: {len(word)}')
    }
}

# No need to '.close()' here! (although, you can if you want to... its a no-op)
# fp.close()


# Now, we open the file for writing text ('w'). This will create the file if it
#   doesn't exit, and wipe it clean if it does already exist
with fp = open('tmp-test.txt', 'w') {
    # Now, you can produce output to the file by calling any IO-compatible function
    # For example, '.print(*args)' works just like the normal 'print' global function
    fp.print('Hey welcome to my test file!')

    # You can also use '.write', which does not behave like 'print' (i.e. takes 1
    #   argument, doesn't add newline, and so forth)
    fp.write('This is just raw text\n\nSo I had to add these newlines\n\n')

}

# You still need to '.close()' if you didn't use a 'with' block
# fp.close()
