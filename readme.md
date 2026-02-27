# Chess Game

Chess implementation with FEN setup, bitboard-based move generation, and legal move highlighting.

## Features

- **FEN String Setup**: Board initialized via `FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR")`
- **Piece Movement** (Pawn, Knight, King):
  - Pawns: forward 1 or 2 from starting rank; diagonal capture
  - Knights: L-shaped jumps (can jump over pieces)
  - Kings: one square in any direction
- **Bitboard Move Generation**: Uses `BitboardElement`, `BitMove`, and precomputed Knight/King tables
- **Capture**: Capturing removes the opponent's piece from the board
- **Legal Move Highlighting**: Highlights valid destination squares when dragging a piece
- **Turn Alternation**: White and Black alternate turns

## Not Implemented (per assignment)

- En Passant
- Castling
- Pawn Promotion

## Build

```
cd build
cmake --build . --config Debug
```
