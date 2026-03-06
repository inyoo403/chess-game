#include "Chess.h"
#include <limits>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace {

uint64_t s_knightMoves[64];
uint64_t s_kingMoves[64];

void initMoveTables() {
    static bool done = false;
    if (done) return;
    done = true;

    const int knightDx[] = { 2, 2, -2, -2, 1, 1, -1, -1 };
    const int knightDy[] = { 1, -1, 1, -1, 2, -2, 2, -2 };
    const int kingDx[] = { 1, 1, 1, 0, 0, -1, -1, -1 };
    const int kingDy[] = { 1, 0, -1, 1, -1, 1, 0, -1 };

    for (int sq = 0; sq < 64; sq++) {
        int x = sq % 8;
        int y = sq / 8;

        s_knightMoves[sq] = 0;
        for (int i = 0; i < 8; i++) {
            int nx = x + knightDx[i];
            int ny = y + knightDy[i];
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
                s_knightMoves[sq] |= (1ULL << (ny * 8 + nx));
        }

        s_kingMoves[sq] = 0;
        for (int i = 0; i < 8; i++) {
            int nx = x + kingDx[i];
            int ny = y + kingDy[i];
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8)
                s_kingMoves[sq] |= (1ULL << (ny * 8 + nx));
        }
    }
}

}

Chess::Chess()
{
    initMoveTables();
    initMagicBitboards();
    _grid = new Grid(8, 8);
}

// Frees magic bitboard memory and deletes the grid
Chess::~Chess()
{
    cleanupMagicBitboards();
    delete _grid;
}

// Returns the character notation for the piece at the given board position
char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

// Creates and returns a new piece sprite for the given player and piece type
Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

// Sets up the initial chess board with all pieces in starting positions
void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

// Parses a FEN string and places pieces on the board accordingly
void Chess::FENtoBoard(const std::string& fen) {
    std::string boardPart = fen.substr(0, fen.find(' '));

    static const std::unordered_map<char, std::pair<int, ChessPiece>> fenMap = {
        {'P', {0, Pawn}},   {'N', {0, Knight}}, {'B', {0, Bishop}},
        {'R', {0, Rook}},   {'Q', {0, Queen}},  {'K', {0, King}},
        {'p', {1, Pawn}},   {'n', {1, Knight}}, {'b', {1, Bishop}},
        {'r', {1, Rook}},   {'q', {1, Queen}},  {'k', {1, King}},
    };

    int x = 0, y = 7;
    for (char c : boardPart) {
        if (c == '/') {
            x = 0;
            y--;
        } else if (c >= '1' && c <= '8') {
            x += (c - '0');
        } else {
            auto it = fenMap.find(c);
            if (it != fenMap.end()) {
                int playerNumber = it->second.first;
                ChessPiece piece = it->second.second;
                Bit* bit = PieceForPlayer(playerNumber, piece);
                bit->setGameTag(playerNumber == 0 ? piece : piece + 128);
                bit->setPosition(_grid->getSquare(x, y)->getPosition());
                _grid->getSquare(x, y)->setBit(bit);
            }
            x++;
        }
    }
}

// Called when an empty square is clicked; returns false since we don't place new pieces
bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

// Highlights legal move squares while dragging a piece, then draws the board
void Chess::drawFrame()
{
    clearBoardHighlights();
    if (_dragBit && _oldHolder) {
        ChessSquare* srcSq = static_cast<ChessSquare*>(_oldHolder);
        int fromIdx = srcSq->getRow() * 8 + srcSq->getColumn();
        auto moves = generateMoves();
        for (const auto& m : moves) {
            if (m.from == fromIdx) {
                int tx = m.to % 8, ty = m.to / 8;
                getGrid()->getSquare(tx, ty)->setHighlighted(true);
            }
        }
    }
    Game::drawFrame();
}

// Removes highlight from all squares on the board
void Chess::clearBoardHighlights()
{
    getGrid()->forEachSquare([](ChessSquare* square, int, int) {
        square->setHighlighted(false);
    });
}

// Scans the board and builds bitboards for white and black piece positions
void Chess::updateBitboards(uint64_t& whiteBB, uint64_t& blackBB) const
{
    whiteBB = 0;
    blackBB = 0;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* b = square->bit();
        if (!b) return;
        uint64_t bit = 1ULL << (y * 8 + x);
        if (b->gameTag() < 128)
            whiteBB |= bit;
        else
            blackBB |= bit;
    });
}

// Generates all legal moves for the current player and returns them as a list
std::vector<BitMove> Chess::generateMoves()
{
    std::vector<BitMove> moves;
    uint64_t whiteBB, blackBB;
    updateBitboards(whiteBB, blackBB);
    uint64_t allPieces = whiteBB | blackBB;

    int currentPlayer = getCurrentPlayer()->playerNumber();
    uint64_t ourPieces = (currentPlayer == 0) ? whiteBB : blackBB;
    uint64_t theirPieces = (currentPlayer == 0) ? blackBB : whiteBB;
    bool isWhite = (currentPlayer == 0);

    BitboardElement ourBB(ourPieces);
    ourBB.forEachBit([&](int from) {
        int x = from % 8, y = from / 8;
        Bit* b = _grid->getSquare(x, y)->bit();
        if (!b) return;
        int tag = b->gameTag();
        ChessPiece pieceType = static_cast<ChessPiece>(tag & 127);

        if (pieceType == Knight) {
            uint64_t targets = s_knightMoves[from] & ~ourPieces;
            BitboardElement t(targets);
            t.forEachBit([&](int to) { moves.push_back(BitMove(from, to, Knight)); });
        }
        else if (pieceType == King) {
            uint64_t targets = s_kingMoves[from] & ~ourPieces;
            BitboardElement t(targets);
            t.forEachBit([&](int to) { moves.push_back(BitMove(from, to, King)); });
        }
        else if (pieceType == Rook) {
            uint64_t targets = getRookAttacks(from, allPieces) & ~ourPieces;
            BitboardElement t(targets);
            t.forEachBit([&](int to) { moves.push_back(BitMove(from, to, Rook)); });
        }
        else if (pieceType == Bishop) {
            uint64_t targets = getBishopAttacks(from, allPieces) & ~ourPieces;
            BitboardElement t(targets);
            t.forEachBit([&](int to) { moves.push_back(BitMove(from, to, Bishop)); });
        }
        else if (pieceType == Queen) {
            uint64_t targets = getQueenAttacks(from, allPieces) & ~ourPieces;
            BitboardElement t(targets);
            t.forEachBit([&](int to) { moves.push_back(BitMove(from, to, Queen)); });
        }
        else if (pieceType == Pawn) {
            if (isWhite) {
                // Forward +1
                if (y < 7) {
                    int to = (y + 1) * 8 + x;
                    if (!(allPieces & (1ULL << to)))
                        moves.push_back(BitMove(from, to, Pawn));
                }
                // Forward +2
                if (y == 1) {
                    int to1 = 2 * 8 + x, to2 = 3 * 8 + x;
                    if (!(allPieces & (1ULL << to1)) && !(allPieces & (1ULL << to2)))
                        moves.push_back(BitMove(from, to2, Pawn));
                }
                // Captures diagonal
                if (y < 7) {
                    if (x > 0) { int to = (y + 1) * 8 + (x - 1); if (theirPieces & (1ULL << to)) moves.push_back(BitMove(from, to, Pawn)); }
                    if (x < 7) { int to = (y + 1) * 8 + (x + 1); if (theirPieces & (1ULL << to)) moves.push_back(BitMove(from, to, Pawn)); }
                }
            } else {
                // Black: forward -1
                if (y > 0) {
                    int to = (y - 1) * 8 + x;
                    if (!(allPieces & (1ULL << to)))
                        moves.push_back(BitMove(from, to, Pawn));
                }
                // Forward +2
                if (y == 6) {
                    int to1 = 5 * 8 + x, to2 = 4 * 8 + x;
                    if (!(allPieces & (1ULL << to1)) && !(allPieces & (1ULL << to2)))
                        moves.push_back(BitMove(from, to2, Pawn));
                }
                // Captures diagonal
                if (y > 0) {
                    if (x > 0) { int to = (y - 1) * 8 + (x - 1); if (theirPieces & (1ULL << to)) moves.push_back(BitMove(from, to, Pawn)); }
                    if (x < 7) { int to = (y - 1) * 8 + (x + 1); if (theirPieces & (1ULL << to)) moves.push_back(BitMove(from, to, Pawn)); }
                }
            }
        }
    });

    return moves;
}

// Checks if the given piece belongs to the current player and has any legal moves
bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    ChessSquare* srcSq = static_cast<ChessSquare*>(&src);
    int fromIdx = srcSq->getRow() * 8 + srcSq->getColumn();

    auto moves = generateMoves();
    for (const auto& m : moves) {
        if (m.from == fromIdx) return true;
    }
    return false;
}

// Checks if moving from src to dst is a legal move for the given piece
bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSq = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSq = static_cast<ChessSquare*>(&dst);
    int fromIdx = srcSq->getRow() * 8 + srcSq->getColumn();
    int toIdx = dstSq->getRow() * 8 + dstSq->getColumn();

    auto moves = generateMoves();
    for (const auto& m : moves) {
        if (m.from == fromIdx && m.to == toIdx) return true;
    }
    return false;
}

// Cleans up by destroying all pieces on the board
void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

// Returns the owner (player) of the piece at the given board coordinates
Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

// Returns the board state string at the start of the game
std::string Chess::initialStateString()
{
    return stateString();
}

// Converts the current board into a string representation
std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

// Restores the board from a previously saved state string
void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
