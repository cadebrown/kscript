#!/usr/bin/env ks

""" chess.ks - pure kscript chess engine

@author: Cade Brown <cade@kscript.org>
"""

# -*- Enumerations/Constants -*-

Color = enum.make('Color', [
    ('WHITE', 0),
    ('BLACK', 1),
])

# Determine the other color
Color.other = x -> 1 - x

Piece = enum.make('Piece', [
    ('KING', 0),
    ('QUEEN', 1),
    ('BISHOP', 2),
    ('KNIGHT', 3),
    ('ROOK', 4),
    ('PAWN', 5),
])

Piece.names_full = {
    (Color.WHITE, Piece.KING): 'WHITE KING',
    (Color.WHITE, Piece.QUEEN): 'WHITE QUEEN',
    (Color.WHITE, Piece.BISHOP): 'WHITE BISHOP',
    (Color.WHITE, Piece.KNIGHT): 'WHITE KNIGHT',
    (Color.WHITE, Piece.ROOK): 'WHITE ROOK',
    (Color.WHITE, Piece.PAWN): 'WHITE PAWN',
    (Color.BLACK, Piece.KING): 'BLACK KING',
    (Color.BLACK, Piece.QUEEN): 'BLACK QUEEN',
    (Color.BLACK, Piece.BISHOP): 'BLACK BISHOP',
    (Color.BLACK, Piece.KNIGHT): 'BLACK KNIGHT',
    (Color.BLACK, Piece.ROOK): 'BLACK ROOK',
    (Color.BLACK, Piece.PAWN): 'BLACK PAWN',
    none: '.',
}

Piece.names_ascii = {
    (Color.WHITE, Piece.KING): 'K',
    (Color.WHITE, Piece.QUEEN): 'Q',
    (Color.WHITE, Piece.BISHOP): 'B',
    (Color.WHITE, Piece.KNIGHT): 'N',
    (Color.WHITE, Piece.ROOK): 'R',
    (Color.WHITE, Piece.PAWN): 'P',
    (Color.BLACK, Piece.KING): 'k',
    (Color.BLACK, Piece.QUEEN): 'q',
    (Color.BLACK, Piece.BISHOP): 'b',
    (Color.BLACK, Piece.KNIGHT): 'n',
    (Color.BLACK, Piece.ROOK): 'r',
    (Color.BLACK, Piece.PAWN): 'p',
    none: '.',
}

Piece.names_ucd = {
    (Color.WHITE, Piece.KING): '\N[WHITE CHESS KING]',
    (Color.WHITE, Piece.QUEEN): '\N[WHITE CHESS QUEEN]',
    (Color.WHITE, Piece.BISHOP): '\N[WHITE CHESS BISHOP]',
    (Color.WHITE, Piece.KNIGHT): '\N[WHITE CHESS KNIGHT]',
    (Color.WHITE, Piece.ROOK): '\N[WHITE CHESS ROOK]',
    (Color.WHITE, Piece.PAWN): '\N[WHITE CHESS PAWN]',
    (Color.BLACK, Piece.KING): '\N[BLACK CHESS KING]',
    (Color.BLACK, Piece.QUEEN): '\N[BLACK CHESS QUEEN]',
    (Color.BLACK, Piece.BISHOP): '\N[BLACK CHESS BISHOP]',
    (Color.BLACK, Piece.KNIGHT): '\N[BLACK CHESS KNIGHT]',
    (Color.BLACK, Piece.ROOK): '\N[BLACK CHESS ROOK]',
    (Color.BLACK, Piece.PAWN): '\N[BLACK CHESS PAWN]',
    none: '.',
}


# -*- Bitboards -*-

# Create a 'hot' bitboard (i.e. a onehot vector of bits)
func bb_hot(idx) {
    if isinst(idx, (str, Tile, tuple)) {
        idx = idx as Tile as int
    }
    ret 1 << idx
}

# Create a mask referencing the tiles between 'fr' and 'to', which should be on
#   a straight line
func mask_grid(fr, to) {
    fr = fr as Tile
    to = to as Tile

    # Decompose into basic points
    (a, b, c, d) = (fr.rank, fr.file, to.rank, to.file)

    # Result mask
    r = 0

    if a == c {
        # Ensure lower is the one that starts
        if b < d, (b, d) = (d, b)

        # Iterate over files
        for i in range(b+1, d), r |= bb_hot(Tile(a, i))

    } else {
        assert b == d

        # Ensure lower is the one that starts
        if a < c, (a, c) = (c, a)

        # Iterate over ranks
        for i in range(a+1, c), r |= bb_hot(Tile(i, b))
    }

    ret r

}

# Create a mask referencing the tiles between 'fr' and 'to', which should
#   be on a diagonal line
func mask_diag(fr, to) {
    fr = fr as Tile
    to = to as Tile

    # Decompose into basic points
    (a, b, c, d) = (fr.rank, fr.file, to.rank, to.file)

    # Result mask
    r = 0
    assert abs(c - a) == abs(d - b)
    
    if a < c, (a, c) = (c, a)
    if b < d, (b, d) = (d, b)

    # add diagonal tiles
    for i in range(a + 1, c), r |= bb_hot(Tile(i, i - a + b))

    ret r
}



# -*- Game Types -*-

# Tile location on the board
type Tile {

    # (files/columns) names
    files = [
        'A',
        'B',
        'C',
        'D',
        'E',
        'F',
        'G',
        'H',
    ]

    # (ranks/rows) anames
    ranks = [
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
    ]

    # create from rank and file
    func __init(self, rank, file=none) {
        if file == none {
            if isinst(rank, str) {
                # Convert from string notation
                (rank, file) = (ranks.index(rank[1]), files.index(rank[0]))
            } elif isinst(rank, int) {
                (rank, file) = (rank // 8, rank % 8)
            } elif isinst(rank, (tuple, list)) {
                (rank, file) = rank
            } elif isinst(rank, Tile) {
                (rank, file) = (rank.rank, rank.file)
            } else {
                throw Error('Failed to convert object of type \'%T\' to \'%T\'' % (rank, self))
            }
        }
        
        (self.rank, self.file) = (rank, file)
    }

    func __str(self) {
        ret "%s%s" % (type(self).files[self.file], type(self).ranks[self.rank],)
    }

    func __repr(self) {
        ret "%T(%R, %R)" % (self, self.rank, self.file,)
    }

    func __eq(L, R) {
        L = L as Tile
        R = R as Tile
        ret L.rank == R.rank && L.file == R.file
    }

    # Turn into an integer, which gives a unique integer for each tile (from 0 to 64)
    func __int(self) {
        ret 8 * self.rank + self.file
    }

    func __hash(self) {
        ret self as int
    }
}

# State of a chess game (includes pieces, as well as flags such as rook/pawn moves, promotion choices, etc)
type State {
    func __init(self, c=none, p=none, promotes=none) {
        # map of colors (bitboards)
        self.c = c || {
            Color.WHITE: 0xFFFF,
            Color.BLACK: 0xFFFF << (64 - 2 * 8),
        }

        # map of pieces (bitboards)
        # this is the default starting configuration
        self.p = p || {
            Piece.KING  : bb_hot('E1') | bb_hot('E8'),
            Piece.QUEEN : bb_hot('D1') | bb_hot('D8'),
            Piece.BISHOP: bb_hot('C1') | bb_hot('F1') | bb_hot('C8') | bb_hot('F8'),
            Piece.KNIGHT: bb_hot('B1') | bb_hot('G1') | bb_hot('B8') | bb_hot('G8'),
            Piece.ROOK  : bb_hot('A1') | bb_hot('H1') | bb_hot('A8') | bb_hot('H8'),
            Piece.PAWN  : bb_hot('A2') | bb_hot('B2') | bb_hot('C2') | bb_hot('D2') | bb_hot('E2') | bb_hot('F2') | bb_hot('G2') | bb_hot('H2') |
                          bb_hot('A7') | bb_hot('B7') | bb_hot('C7') | bb_hot('D7') | bb_hot('E7') | bb_hot('F7') | bb_hot('G7') | bb_hot('H7'),
        }

        # Promotion selection (used when a promotion occurs)
        self.promotes = promotes || {
            Color.WHITE: Piece.QUEEN,
            Color.BLACK: Piece.QUEEN,
        }
    }

    func __repr(self) {
        ret '%T(%r, %r)' % (self, self.c, self.p)
    }

    func __str(self) {
        res = ''
        for r in range(8) {
            res += '|'
            for f in range(8) {
                res += '%s|' % (Piece.names_ucd[self[(r, f)]],)
            }
            res += '\n'
        }


        ret res
    }

    # Gets a tile index
    func __getelem(self, key) {
        key = key as Tile as int
        # Check colors
        for c in self.c {
            m = bb_hot(key)
            if self.c[c] & m {
                # Check which piece it is
                for p in self.p {
                    if self.p[p] & m {
                        ret (c, p)
                    }
                }
            }
        }

        # not found
        ret none
    }
    
    # Hash board state
    func __hash(self) {
        res = 0
        for k in self.c {
            res ^= self.c[k]
        }

        # Shift off to avoid cancelation
        res <<= 13

        for k in self.p {
            res ^= self.p[k]
        }

        ret res
    }

    # -*- Low-level Manipulation -*-

    # Add a color+piece to a tile
    # NOTE: don't use this, as this is a low-level function that does not check for validity
    func add(self, pos, c, p) {
        pos = pos as Tile

        # Mask for the position being edited
        mask = bb_hot(pos)
        
        # Update the color map
        self.c[c] |= mask
        self.c[c.other()] &= ~mask

        # Update the piecces
        self.p[p] |= mask
    }

    # Clear a tile
    # NOTE: don't use this, as this is a low-level function that does not check for validity
    func rem(self, pos) {
        pos = pos as Tile

        # Mask for the position being edited
        mask = bb_hot(pos)

        self.c[Color.WHITE] &= ~mask
        self.c[Color.BLACK] &= ~mask

        for k in self.p {
            self.p[k] &= ~mask
        }
    }

    # -*- High-level Manipulation -*-

    # Apply a move
    func move(self, fr, to) {
        fr = fr as Tile
        to = to as Tile
        assert self.isvalid(fr, to)

        # Masks for locations
        mf = bb_hot(fr)
        mt = bb_hot(to)

        # Get the piece being moved
        (c, p) = self[fr]

        # Handle special cases
        if p == Piece.PAWN && ((c == Color.WHITE && to.rank == 7)|| (c == Color.BLACK && to.rank == 0)) {
            # Pawn promotion
            newp = self.promotes[c]

            # Replace the colors
            s.c[c] = (s.c[c] & ~mf) | mt
            s.c[c.other()] &= ~mf & ~mt

            # Replace pieces maps
            for k in self.p {
                self.p[k] &= ~mf & ~mt
            }
            # Add back to the one moved
            self.p[newp] |= mt

        } else {
            # Replace the colors
            s.c[c] = (s.c[c] & ~mf) | mt
            s.c[c.other()] &= ~mf & ~mt

            # Replace pieces maps
            for k in self.p {
                self.p[k] &= ~mf & ~mt
            }
            # Add back to the one moved
            self.p[p] |= mt
        }
    }

    # -*- Analysis -*-

    # Check if the move 'fr -> to' is valid
    func isvalid(self, fr, to) {
        fr = fr as Tile
        to = to as Tile

        # Can't move to the same square
        if fr == to, ret false

        # Check the piece we are moving from
        cp = self[fr]
        if !cp, ret false
        (c, p) = cp

        # Get the other color
        oc = c.other()

        # Masks
        mf = bb_hot(fr)
        mt = bb_hot(to)

        # Can't move to where one of our own pieces is
        if self.c[c] & mt, ret false

        # Whether or not the piece can move there
        good = false

        if p == Piece.KING {
            # Can move in 8 squares around it (self-move has already been
            #   ruled out)
            good = abs(to.rank - fr.rank) < 2 && abs(to.file - fr.file) < 2
        } elif p == Piece.QUEEN {
            # Can move on grid or diagonal
            if fr.rank == to.rank || fr.file == to.file {
                pm = mask_grid(fr, to)
                good = !(s.c[Color.WHITE] & pm) && !(s.c[Color.BLACK] & pm)
            } elif abs(fr.rank - to.rank) == abs(fr.file - to.file) {
                pm = mask_diag(fr, to)
                good = !(s.c[Color.WHITE] & pm) && !(s.c[Color.BLACK] & pm)
            }
        } elif p == Piece.BISHOP {
            # Can move on diagonal
            if abs(fr.rank - to.rank) == abs(fr.file - to.file) {
                pm = mask_diag(fr, to)
                good = !(s.c[Color.WHITE] & pm) && !(s.c[Color.BLACK] & pm)
            }
        } elif p == Piece.KNIGHT {
            # Can move in L-shape, with sides of 2 and 1
            ar = abs(to.rank - fr.rank)
            af = abs(to.file - fr.file)
            good = (ar == 2 && af == 1) || (ar == 1 && af == 2)
        } elif p == Piece.ROOK {
            # Can move on grid
            if fr.rank == to.rank || fr.file == to.file {
                pm = mask_grid(fr, to)
                good = !(s.c[Color.WHITE] & pm) && !(s.c[Color.BLACK] & pm)
            } 
        } elif p == Piece.PAWN {
            # Pawns depend on their color, so we must have seperate blocks
            if c == Color.WHITE {
                if fr.file == to.file {
                    # Straight movement forward
                    if fr.rank == 1 {
                        good = to.rank in (2, 3) && !(s.c[oc] & mt)
                    } else {
                        good = to.rank == fr.rank + 1 && !(s.c[oc] & mt)
                    }
                } elif to.file in (fr.file + 1, fr.file - 1) && to.rank == fr.rank + 1 {
                    # Diagonal move (so it must capture an enemy)
                    good = s.c[oc] & mt
                }
            } else {
                if fr.file == to.file {
                    # Straight movement forward
                    if fr.rank == 6 {
                        good = to.rank in (4, 5) && !(s.c[oc] & mt)
                    } else {
                        good = to.rank == fr.rank - 1 && !(s.c[oc] & mt)
                    }
                } elif to.file == fr.file + 1 && to.rank == fr.rank - 1 {
                    # Diagonal move (so it must capture an enemy)
                    good = s.c[oc] & mt
                }
            }
        } else {
            assert false
        }

        # If it was not possible to move there, exit early
        if !good, ret false

        # Now, check and make sure we are not pinned (i.e. moving
        #   would be illegal because we leave the king in check)
        # TODO: add method to detect this


        # Success, we can move
        ret true
    }

    # Return a string representing a move
    func strmove(self, fr, to) {
        pto = self[to]
        ret '%s from %s to %s %s' % (Piece.names_full[self[fr]], fr, to, '(cap %s)' % (Piece.names_full[pto], ) if pto else '')
    }

    # Returns a list of valid moves
    func moves(self, c) {
        res = []
        oc = c.other()

        # Iterate over locations on the board
        for b in range(64) {

            m = bb_hot(b)
            if s.c[c] & m {
                # This tile contains a piece that is our color,
                #   so tell which piece
                for p in self.p {
                    if self.p[p] & m, break
                }

                # Get rank and file
                bt = b as Tile
                (fr, ff) = (bt.rank, bt.file)

                # Now, define a function to emit (possible)
                #   positions by checking whether they are valid
                func emit(tr, tf) {
                    if tr < 0 || tr >= 8 || tf < 0 || tf >= 8, ret false
                    x = b as Tile
                    y = (tr, tf) as Tile
                    if !self.isvalid(x, y), ret false

                    # Otherwise, it's good so add it
                    res.push((x, y))
                    ret true
                }

                # Emit the grid around the current position
                func emit_grid() {
                    for i in range(fr + 1, 8) {
                        (tr, tf) = (i, ff)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(fr - 1, -1, -1) {
                        (tr, tf) = (i, ff)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(ff + 1, 8) {
                        (tr, tf) = (fr, i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(ff - 1, -1, -1) {
                        (tr, tf) = (fr, i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                }
            
                # Emit the diagonal around the current position
                func emit_diag() {
                    for i in range(1, 8) {
                        if fr + i >= 8 || ff + i >= 8, break
                        (tr, tf) = (fr + i, ff + i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(1, 8) {
                        if fr - i < 0 || ff - i < 0, break
                        (tr, tf) = (fr - i, ff - i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            break
                        } elif s.c[oc] & pm {
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(1, 8) {
                        if fr + i >= 8 || ff - i < 0, break
                        (tr, tf) = (fr + i, ff - i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            # Found a friendly square
                            break
                        } elif s.c[oc] & pm {
                            # Found an enemy square, so allow capture, but no further
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                    for i in range(1, 8) {
                        if fr - i < 0 || ff + i >= 8, break
                        (tr, tf) = (fr - i, ff + i)
                        pm = bb_hot((tr, tf))
                        if s.c[c] & pm {
                            break
                        } elif s.c[oc] & pm {
                            emit(tr, tf)
                            break
                        }
                        emit(tr, tf)
                    }
                }
                if p == Piece.KING {
                    # Can move in 8 adjacent squares
                    emit(fr-1, ff-1)
                    emit(fr-1, ff)
                    emit(fr-1, ff+1)
                    emit(fr, ff-1)
                    emit(fr, ff+1)
                    emit(fr+1, ff-1)
                    emit(fr+1, ff)
                    emit(fr+1, ff+1)
                } elif p == Piece.QUEEN {
                    # Can move in grid or diagonal
                    emit_grid()
                    emit_diag()
                } elif p == Piece.BISHOP {
                    # Can move in diagonal
                    emit_diag()
                } elif p == Piece.KNIGHT {
                    # Can move in L-shape
                    emit(fr+1, ff+2)
                    emit(fr+1, ff-2)
                    emit(fr-1, ff+2)
                    emit(fr-1, ff-2)
                    emit(fr+2, ff+1)
                    emit(fr+2, ff-1)
                    emit(fr-2, ff+1)
                    emit(fr-2, ff-1)
                } elif p == Piece.ROOK {
                    # Can move in grid
                    emit_grid()
                } elif p == Piece.PAWN {
                    # Can move forward, or capture immediate diagonal
                    if c == Color.WHITE {
                        emit(fr+1, ff)
                        if fr == 1, emit(fr+2, ff)

                        emit(fr+1, ff+1)
                        emit(fr+1, ff-1)
                    } else {
                        emit(fr-1, ff)
                        if fr == 6, emit(fr-2, ff)

                        emit(fr-1, ff+1)
                        emit(fr-1, ff-1)
                    }
                }
            }
        }

        ret res
    }
}

s = State()
print (s)

for i in range(10) {
    moves = s.moves(Color.WHITE)
    for m in moves, print (s.strmove(*m))
    mov = moves[0]
    s.move(*mov)
    print (s)

    moves = s.moves(Color.BLACK)
    for m in moves, print (s.strmove(*m))

    mov = moves[0]
    s.move(*mov)
    print (s)
}

