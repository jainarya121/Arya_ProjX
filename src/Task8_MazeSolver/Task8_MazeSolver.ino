const char *MAZE[] = {
  "  B-G-C",
  "  | | |",
  "  | E |",
  "  |   |",
  "D-A---F",
  "  |    ",
  "  S    ",
};
const int MAZE_ROWS = sizeof(MAZE) / sizeof(MAZE[0]);
const int START_ROW = 6;  
const int START_COL = 2;
const int STEP_CM   = 30;

const int LED_PIN = 2;    

const int NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3;
const char *DIR_NAME[] = { "N", "E", "S", "W" };

int leftOf(int d)  { return (d + 3) % 4; }
int rightOf(int d) { return (d + 1) % 4; }
int backOf(int d)  { return (d + 2) % 4; }

const char *turnName(int facing, int newDir) {
  if (newDir == facing)          return "STRAIGHT";
  if (newDir == leftOf(facing))  return "LEFT";
  if (newDir == rightOf(facing)) return "RIGHT";
  return "U-TURN";
}

int heading = NORTH;    
int robotRow, robotCol; 

int  foundCm;           
int  foundX, foundY;   
bool foundExits[4];     
bool foundEnd; 
char foundName; 

const int ROW_STEP[4] = { -1, 0, 1, 0 };  
const int COL_STEP[4] = { 0, 1, 0, -1 };

char mazeAt(int row, int col) {
  if (row < 0 || row >= MAZE_ROWS) return ' ';
  if (col < 0 || col >= (int)strlen(MAZE[row])) return ' ';
  return MAZE[row][col];
}

void robotGoToStart() {
  robotRow = START_ROW;
  robotCol = START_COL;
  heading = NORTH;
}

void robotTurnTo(int newDir) {
  heading = newDir;
}

void robotDriveToNextNode() {
  foundCm = 0;
  while (true) {
    robotRow = robotRow + 2 * ROW_STEP[heading];
    robotCol = robotCol + 2 * COL_STEP[heading];
    foundCm = foundCm + STEP_CM;

    int lines = 0;
    for (int d = 0; d < 4; d++) {
      foundExits[d] = mazeAt(robotRow + ROW_STEP[d], robotCol + COL_STEP[d]) != ' ';
      if (foundExits[d]) lines++;
    }
    foundEnd = (mazeAt(robotRow, robotCol) == 'E');

    if (foundEnd || lines != 2) break;

    int cameFrom = backOf(heading);
    for (int d = 0; d < 4; d++) {
      if (foundExits[d] && d != cameFrom) heading = d;
    }
  }
  foundX = (robotCol - START_COL) / 2 * STEP_CM;
  foundY = (START_ROW - robotRow) / 2 * STEP_CM;
  foundName = mazeAt(robotRow, robotCol);
}

struct Node {
  int  x, y;
  bool exit[4];
  bool tried[4];
  char name;
};

struct Edge {
  int a, b; 
  int cm; 
  int dirFromA;  
  int dirFromB; 
};

Node nodes[50];
int  nodeCount = 0;
Edge edges[80];
int  edgeCount = 0;
int  startNode, endNode;

int addNode() {
  int i = nodeCount;
  nodes[i].x = foundX;
  nodes[i].y = foundY;
  nodes[i].name = foundName;
  for (int d = 0; d < 4; d++) {
    nodes[i].exit[d]  = foundExits[d];
    nodes[i].tried[d] = false;
  }
  nodeCount++;
  return i;
}

int findNode(int x, int y) {
  for (int i = 0; i < nodeCount; i++) {
    if (abs(nodes[i].x - x) <= 10 && abs(nodes[i].y - y) <= 10) return i;
  }
  return -1; 
}

void addEdge(int a, int dirFromA, int b, int dirFromB, int cm) {
  edges[edgeCount].a = a;
  edges[edgeCount].b = b;
  edges[edgeCount].cm = cm;
  edges[edgeCount].dirFromA = dirFromA;
  edges[edgeCount].dirFromB = dirFromB;
  edgeCount++;
  nodes[a].tried[dirFromA] = true;
  nodes[b].tried[dirFromB] = true;
}

int otherEnd(int e, int n) { return (edges[e].a == n) ? edges[e].b : edges[e].a; }
int dirFrom(int e, int n)  { return (edges[e].a == n) ? edges[e].dirFromA : edges[e].dirFromB; }

int pickNewLine(int n) {
  int order[4] = { leftOf(heading), heading, rightOf(heading), backOf(heading) };
  for (int i = 0; i < 4; i++) {
    int d = order[i];
    if (nodes[n].exit[d] && !nodes[n].tried[d]) return d;
  }
  return -1;
}

int newestNodeWithNewLine() {
  for (int i = nodeCount - 1; i >= 0; i--) {
    for (int d = 0; d < 4; d++) {
      if (nodes[i].exit[d] && !nodes[i].tried[d]) return i;
    }
  }
  return -1;
}

int routeEdges[50];
int routeLength = 0;

int findShortestRoute(int from, int to) {
  int  dist[50];  
  int  cameBy[50]; 
  bool done[50];
  for (int i = 0; i < nodeCount; i++) {
    dist[i] = 999999;
    cameBy[i] = -1;
    done[i] = false;
  }
  dist[from] = 0;

  while (true) {
    int u = -1;
    for (int i = 0; i < nodeCount; i++) {
      if (!done[i] && dist[i] < 999999 && (u == -1 || dist[i] < dist[u])) u = i;
    }
    if (u == -1) break; 
    done[u] = true;

    for (int e = 0; e < edgeCount; e++) {
      if (edges[e].a != u && edges[e].b != u) continue;
      int v = otherEnd(e, u);
      if (dist[u] + edges[e].cm < dist[v]) {
        dist[v] = dist[u] + edges[e].cm;
        cameBy[v] = e;
      }
    }
  }

  routeLength = 0;
  for (int n = to; n != from; n = otherEnd(cameBy[n], n)) {
    routeEdges[routeLength] = cameBy[n];
    routeLength++;
  }
  for (int i = 0; i < routeLength / 2; i++) {
    int t = routeEdges[i];
    routeEdges[i] = routeEdges[routeLength - 1 - i];
    routeEdges[routeLength - 1 - i] = t;
  }
  return dist[to];
}

void driveRoute(int from) {
  int n = from;
  for (int i = 0; i < routeLength; i++) {
    int e = routeEdges[i];
    robotTurnTo(dirFrom(e, n));
    robotDriveToNextNode();
    n = otherEnd(e, n);
  }
}

void dryRun() {
  Serial.println("\n=== DRY RUN: explore, left first ===");
  robotGoToStart();

  for (int d = 0; d < 4; d++) foundExits[d] = (d == heading);
  foundX = 0;
  foundY = 0;
  foundName = 'S';
  startNode = addNode();

  int here = startNode;
  int totalCm = 0;

  while (true) {
    int go = pickNewLine(here);

    if (go != -1) {
      Serial.printf("%c: %s, ", nodes[here].name, turnName(heading, go));
      robotTurnTo(go);
      robotDriveToNextNode();
      totalCm = totalCm + foundCm;
      int cameInBy = backOf(heading); 

      int there = findNode(foundX, foundY);
      bool seenBefore = (there != -1);
      if (!seenBefore) there = addNode();
      addEdge(here, go, there, cameInBy, foundCm);

      Serial.printf("drive %d cm to %c", foundCm, nodes[there].name);
      if (foundEnd) {
        Serial.println(" = END box! LED on.");
        digitalWrite(LED_PIN, HIGH);
        endNode = there;
        break;
      }
      if (seenBefore)                 Serial.println(" (been here before: loop found)");
      else if (pickNewLine(there) == -1) Serial.println(" (dead end)");
      else                            Serial.println(" (new junction)");
      here = there;

    } else {
      int target = newestNodeWithNewLine();
      if (target == -1) {
        Serial.println("Explored everything, no END found.");
        return;
      }
      int cm = findShortestRoute(here, target);
      driveRoute(here);
      totalCm = totalCm + cm;
      Serial.printf("%c: nothing new, go back to %c (%d cm)\n", nodes[here].name, nodes[target].name, cm);
      here = target;
    }
  }
  Serial.printf("Dry run total: %d cm\n", totalCm);

  Serial.println("Map:");
  for (int e = 0; e < edgeCount; e++) {
    Serial.printf("  %c - %c : %d cm\n", nodes[edges[e].a].name, nodes[edges[e].b].name, edges[e].cm);
  }
}

int turnDir[50];
int lineCm[50];
int steps = 0;

void plan() {
  Serial.println("\n=== PLAN: shortest route ===");
  int total = findShortestRoute(startNode, endNode);

  int n = startNode;
  Serial.printf("%c", nodes[n].name);
  for (int i = 0; i < routeLength; i++) {
    int e = routeEdges[i];
    turnDir[i] = dirFrom(e, n);
    lineCm[i]  = edges[e].cm;
    n = otherEnd(e, n);
    Serial.printf(" -> %c", nodes[n].name);
  }
  steps = routeLength;
  Serial.printf(" = %d cm\n", total);
}

void actualRun() {
  Serial.println("\n=== ACTUAL RUN: follow the list ===");
  robotGoToStart();
  int totalCm = 0;

  for (int i = 0; i < steps; i++) {
    Serial.printf("Stop %d: %s", i + 1, turnName(heading, turnDir[i]));
    robotTurnTo(turnDir[i]);
    robotDriveToNextNode(); 
    totalCm = totalCm + foundCm;
    Serial.printf(", drove %d cm (expected %d)\n", foundCm, lineCm[i]);
  }
  digitalWrite(LED_PIN, HIGH);
  Serial.printf("END reached, LED on. Total: %d cm\n", totalCm);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dryRun();
  plan();
  digitalWrite(LED_PIN, LOW);
  actualRun();
}

void loop() {
}
