#pragma once

struct RoomOpenings;

// Static Milestone 2 furniture models, expressed in the room's metre units.
void drawFurniture(bool picking = false);

enum class FurnitureId { Bed, Desk, Chair, Wardrobe, Rug, Door, Window, Bookshelf };

// Color-picking pass returns -1 for empty space or an editable object index.
void selectFurnitureById(int id);
const char* selectedFurnitureName();
float selectedFurnitureScale();
float selectedFurnitureScaleY();
int selectedFurnitureId();
RoomOpenings editableRoomOpenings();
void moveSelectedFurniture(float dx, float dz);
void rotateSelectedFurniture(float degrees);
void scaleSelectedFurniture(float amount);
void scaleSelectedFurnitureY(float amount);
void resetSelectedFurniture();
void resetAllFurniture();
