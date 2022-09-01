# SUFF
Code of the SUFF framework
## Compile
It requires CMake, g++11, and gperftools to compile, please be sure to install them in your system.
Then just run build.sh to compile the code.

## Data
Graphs files should be a text file of the following format:

```
t 4 3
v 0 14 1
v 1 10 2
v 2 14 2
v 3 0 1
e 0 1
e 1 2
e 2 3
```
The first line starts with a `t`, and then the number of nodes (*n*) and edges (*m*) in this graph.

Then *n* lines, each starts with a `v`, and then the id, label, and degree of a node.

Then *m* lines, each starts with an `e`, and then the start_node_id, end_node_id of an edge.

**Note that node id must be 0~n-1.**

## Usage
Binary files are under the `bin` directory.

For DP-iso, use smatch_dp, otherwise use smatch.

### Normal match
Example: `../bin/smatch -d human.graph -q square.graph -filter CECI -order CECI -engine CECI -num MAX -suff_k 0`

Parameters:
|  option   | meaning  |
|  ----  | ----  |
| -d  | data graph file |
| -q  | query graph file |
| -num  | maximum matching number, "MAX" indicates not limited |
| -suff_k  | k in the paper; k=0 means no SUFF (the original algorithm). |
| -createf  | whether build filters using the results, "1" means yes; ignoring this option means "no". |
| -alpha  | alpha in the paper; ignoring this option will not run filter removal |

By combing these options you can get those methods we compare in the paper:
|  method   | option combination  |
|  ----  | ----  |
| CFL  | -filter CFL -order CFL -engine EXPLORE |
| QuickSI  | -filter LDF -order QSI -engine QSI |
| GraphQL  | -filter GQL -order GQL -engine GQL |
| CECI  | -filter CECI -order CECI -engine CECI |
| DP-iso  | -filter DPiso -order DPiso -engine DPiso |
| VF2++  | -filter LDF -order VF2PP -engine QSI |
