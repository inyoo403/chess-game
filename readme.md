# Chess Game

Chess implementation with FEN setup, bitboard-based move generation, and legal move highlighting.

## Features

- **FEN String Setup**: Board initialized via `FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR")`
- **All Piece Movement**:
  - Pawns: forward 1 or 2 from starting rank; diagonal capture
  - Knights: L-shaped jumps using precomputed bitboard tables
  - Bishops: diagonal sliding moves using magic bitboards
  - Rooks: horizontal/vertical sliding moves using magic bitboards
  - Queens: combined rook + bishop moves using magic bitboards
  - Kings: one square in any direction using precomputed bitboard tables
- **Bitboard Move Generation**: Uses `BitboardElement`, `BitMove`, precomputed Knight/King tables, and magic bitboards for sliding pieces
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
