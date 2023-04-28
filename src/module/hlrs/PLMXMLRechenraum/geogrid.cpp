/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/model.h"
#include <math.h>
#include <vector>

#include <Parser/PLMXMLParser.h>

//#include "include/IOutil.h"

#include "PLMXMLRechenraum.h"

#define sqr(x) (x * x)

const char *plmxml_file;

DOMDocument *document;
PLMXMLParser myParser;

DOMNodeList *listInst;
DOMNodeList *listUserData;
DOMNodeList *listBounds;

XMLCh *TAG_inlet = XMLString::transcode("inlet");
XMLCh *TAG_Inlet = XMLString::transcode("Inlet");
XMLCh *TAG_type = XMLString::transcode("type");
XMLCh *TAG_outlet = XMLString::transcode("outlet");
XMLCh *TAG_Outlet = XMLString::transcode("Outlet");
XMLCh *TAG_transform = XMLString::transcode("Transform");
XMLCh *TAG_name = XMLString::transcode("name");
XMLCh *TAG_RevisionView = XMLString::transcode("ProductRevisionView");
XMLCh *TAG_ProductInstance = XMLString::transcode("ProductInstance");
XMLCh *TAG_id = XMLString::transcode("id");
XMLCh *TAG_DEBBLevel = XMLString::transcode("DEBBLevel");
XMLCh *TAG_UserData = XMLString::transcode("UserData");
XMLCh *TAG_title = XMLString::transcode("title");
XMLCh *TAG_value = XMLString::transcode("value");
XMLCh *TAG_values = XMLString::transcode("values");
XMLCh *TAG_Bound = XMLString::transcode("Bound");

XMLCh *TAG_4holes = XMLString::transcode("IN_FLOOR_4HOLES"); //type 1
XMLCh *TAG_type1 = XMLString::transcode("type1");

XMLCh *TAG_4holesopen = XMLString::transcode("IN_FLOOR_4HOLESOPEN"); //type 2
XMLCh *TAG_type2 = XMLString::transcode("type2");

XMLCh *TAG_lochblech = XMLString::transcode("IN_FLOOR_LOCHBLECH"); //type 3
XMLCh *TAG_type3 = XMLString::transcode("type3");

XMLCh *TAG_1hole = XMLString::transcode("IN_FLOOR_1HOLE"); //type 4
XMLCh *TAG_type4 = XMLString::transcode("type4");

XMLCh *TAG_ceil = XMLString::transcode("ceil");
XMLCh *TAG_ceilx = XMLString::transcode("ceilx");
XMLCh *TAG_ceily = XMLString::transcode("ceily");

struct rechgrid *
CreateRechGrid(struct rech_model *model)
{
    struct rechgrid *grid = new struct rechgrid;

    grid->e = AllocElementStruct();
    grid->p = AllocPointStruct();

    float spacing;
    float x, y, z;
    float xmin = 0.0; //- 0.5*model->size[0];
    float ymin = 0.0; // - 0.5*model->size[1];
    float zmin = 0.0;

    int i, j, k;

    // Create Coordinates
    spacing = 0.6f / model->spacing; // one flor square is 0.6 m

    /* archflow
	 grid->nelem_x = (int) (model->size[0]/spacing);
	 grid->nelem_y = (int) (model->size[1]/spacing);
	 grid->nelem_z = (int) (model->size[2]/spacing);

	 grid->spacing_x = model->size[0]/grid->nelem_x;
	 grid->spacing_y = model->size[1]/grid->nelem_y;
	 grid->spacing_z = model->size[2]/grid->nelem_z;
	 */

    // different to archflow: the spacing must be reliable!
    grid->spacing_x = 0.6f / model->spacing;
    grid->spacing_y = 0.6f / model->spacing;
    grid->spacing_z = 0.6f / model->spacing;

    grid->nelem_x = (int)(model->size[0] / spacing) + 1; // make it a bit bigger, will be cut off to fit later
    grid->nelem_y = (int)(model->size[1] / spacing) + 1;
    grid->nelem_z = (int)(model->size[2] / spacing) + 1;

    grid->npoi_x = grid->nelem_x + 1;
    grid->npoi_y = grid->nelem_y + 1;
    grid->npoi_z = grid->nelem_z + 1;
    grid->nelem = grid->nelem_x * grid->nelem_y * grid->nelem_z;

    fprintf(stderr, "grid size: %d elements, %d nodes\n", grid->nelem,
            grid->npoi_x * grid->npoi_y * grid->npoi_z);

    if (grid->nelem > 10000000)
    {
        fprintf(stderr,
                "Grid is too fine. Please enlarge spacing parameter.\n");
        //return NULL;
    }
    if (grid->nelem < 10000)
    {
        fprintf(stderr,
                "Grid is too coarse. Please reduce spacing parameter.\n");
        return NULL;
    }

    for (i = 0; i < grid->npoi_z; i++)
    {
        for (j = 0; j < grid->npoi_y; j++)
        {
            for (k = 0; k < grid->npoi_x; k++)
            {
                x = xmin + k * grid->spacing_x;
                y = ymin + j * grid->spacing_y;
                z = zmin + i * grid->spacing_z;
                AddPoint(grid->p, x, y, z);
            }
        }
    }

    // Create connectvity list
    //int jmax = grid->nelem_y;
    //int kmax = grid->nelem_x;
    int elem[8];

    for (i = 0; i < grid->nelem_z; i++)
    {
        for (j = 0; j < grid->nelem_y; j++)
        {
            for (k = 0; k < grid->nelem_x; k++)
            {
                elem[0] = i * (grid->npoi_y * grid->npoi_x) + j * grid->npoi_x
                          + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + grid->npoi_x;
                elem[3] = elem[0] + grid->npoi_x;
                elem[4] = elem[0] + (grid->npoi_y * grid->npoi_x);
                elem[5] = elem[1] + (grid->npoi_y * grid->npoi_x);
                elem[6] = elem[2] + (grid->npoi_y * grid->npoi_x);
                elem[7] = elem[3] + (grid->npoi_y * grid->npoi_x);
                AddElement(grid->e, elem);
            }
        }
    }

    // subtract elements from grid and generate bcs
    grid->npoi = grid->p->nump;
    grid->p_to_remove = (int *)calloc(grid->npoi, sizeof(int));
    grid->e_to_remove = (int *)calloc(grid->nelem, sizeof(int));

    grid->bcwall = AllocIlistStruct(100);
    grid->bcwallpol = AllocIlistStruct(25);
    grid->bcwallvol = AllocIlistStruct(25);
    grid->bcwallvol_outer = AllocIlistStruct(25);

    grid->bcin = AllocIlistStruct(100);
    grid->bcinpol = AllocIlistStruct(25);
    //grid->bcinvol = AllocIlistStruct(25);
    grid->bcinvol_outer = AllocIlistStruct(25);

    grid->bcout = AllocIlistStruct(100);
    grid->bcoutpol = AllocIlistStruct(25);
    grid->bcoutvol = AllocIlistStruct(25);
    grid->bcoutvol_outer = AllocIlistStruct(25);

    // BCs of the floor
    if (GenerateFloorBCs(grid, model) == -1)
    {
        return NULL;
    }
    // BCs of the ceiling
    GenerateCeilingBCs(grid, model);

    // the BCs of the surrounding geometry
    DefineBCs(grid, model, -1);

    GenerateBCs(grid, model, -1);

    //fprintf(stderr,"grid->bcwall->num=%d\n",grid->bcwall->num);
    fprintf(stderr, "createRechGrid\n");
    //	fprintf(stderr,"model->nobjects=%d\n",model->nobjects);

    fprintf(stderr, "now the racks\n");
    // the racks
    for (i = 0; i < model->nobjects; i++)
    {
        if (model->cubes[i]->size[0] > 0)
        {
            subtractCube(grid, model, i);
            DefineBCs(grid, model, i);
            GenerateBCs(grid, model, i);
            //		fprintf(stderr,"grid->bcwall->num=%d\n",grid->bcwall->num);
        }
    }

    // clean grid coordinate and connectivity list due to removed cells and vertexes
    // 1. remove points
    int offset = 0;
    for (i = 0; i < grid->npoi; i++)
    {
        if (grid->p_to_remove[i] == 0)
        {
            grid->p->x[i - offset] = grid->p->x[i];
            grid->p->y[i - offset] = grid->p->y[i];
            grid->p->z[i - offset] = grid->p->z[i];
        }
        else if (grid->p_to_remove[i] == -1)
        {
            offset++;
        }
        else
        {
            fprintf(stderr, "error 1 in CreateRechGrid!\n");
        }
    }
    grid->p->nump -= offset;

    // 3. node replacement preparation
    offset = 0;
    grid->new_node = new int[grid->npoi];
    for (i = 0; i < grid->npoi; i++)
    {
        if (grid->p_to_remove[i] == -1)
        {
            grid->new_node[i] = -1;
            offset++;
        }
        else if (grid->p_to_remove[i] == 0)
        {
            grid->new_node[i] = i - offset;
        }
        else
        {
            fprintf(stderr, "error 2 in CreateRechGrid!\n");
        }
    }

    // 2. remove elements
    offset = 0;
    for (i = 0; i < grid->nelem; i++)
    {
        if (grid->e_to_remove[i] == 0)
        {
            for (j = 0; j < 8; j++)
            {
                grid->e->e[i - offset][j] = grid->e->e[i][j];
            }
        }
        if (grid->e_to_remove[i] == -1)
        {
            offset++;
        }
    }
    grid->e->nume -= offset;

    // 4. vertex replacement in element list
    int vertex;
    for (i = 0; i < grid->e->nume; i++)
    {
        for (j = 0; j < 8; j++)
        {
            vertex = grid->e->e[i][j];
            //if ( (grid->new_node[vertex]==-1)&&(grid->e_to_remove[i]!=-1) )
            if ((grid->new_node[vertex] == -1)
                && (grid->e_to_remove[i] != -1))
            {
                fprintf(stderr,
                        "error 3 in CreateRechGrid. Element list uses node that was removed.\n");
            }
            else
            {
                grid->e->e[i][j] = grid->new_node[vertex];
            }
        }
    }

    // 5. mapping list old element nr. -> new element number
    offset = 0;
    grid->new_elem = new int[grid->nelem];
    for (i = 0; i < grid->nelem; i++)
    {
        if (grid->e_to_remove[i] == -1)
        {
            grid->new_elem[i] = -1;
            offset++;
        }
        else if (grid->e_to_remove[i] == 0)
        {
            grid->new_elem[i] = i - offset;
        }
        else
        {
            fprintf(stderr, "error 4 in CreateRechGrid!\n");
        }
    }

    fprintf(stderr, "grid size: %d elements, %d nodes\n", grid->e->nume,
            grid->p->nump);

    // 6. remove walls where we have node-bcs (ven or airconditioning)
    // rechenraum: no node bcs, already treated airconditioning

    // generate list (length: all nodes) that tells us which nodes are used for node-bcs
    // if all nodes of a wall element are used for node-bcs, we must remove this wall element

    // zusaetzlich ueberpruefen: wurde der Knoten bereits entfernt? Das waere eine moegliche Fehlerquelle
    int *isbcnode = (int *)calloc(grid->npoi, sizeof(int));

    int wallnodes[4];
    int wallsremoved = 0;
    int inner_removed;
    int outer_removed;
    int remove;
    int nremoved;
    for (i = 0; i < grid->bcwallvol->num; i++)
    {
        inner_removed = 0;
        outer_removed = 0;
        remove = 0;

        wallnodes[0] = grid->bcwall->list[4 * i];
        wallnodes[1] = grid->bcwall->list[4 * i + 1];
        wallnodes[2] = grid->bcwall->list[4 * i + 2];
        wallnodes[3] = grid->bcwall->list[4 * i + 3];

        // Vergleich ueber zugehoerige Element!!!
        // entfernen, falls beide zugehoerigen Volumenelement (innen und aussen)
        // entfernt wurden oder nicht existieren (= ausserhalb Geometrie)
        if (grid->bcwallvol->list[i] == -1)
            inner_removed = 1;
        /*        else if (grid->e_to_remove[grid->bcwallvol->list[i]]==-1)
		 inner_removed=1;
		 if (grid->e_to_remove[grid->bcwallvol_outer->list[i]]==-1)
		 outer_removed=1;

		 // remove if both volume elements don't exist any more
		 if ( (inner_removed==1) && (outer_removed==1) )
		 {
		 remove=1;
		 }
		 */

        if (inner_removed == 1)
            remove = 1;

        // remove if a wallnode has been removed
        if ((grid->new_node[wallnodes[0]] == -1)
            || (grid->new_node[wallnodes[1]] == -1)
            || (grid->new_node[wallnodes[2]] == -1)
            || (grid->new_node[wallnodes[3]] == -1))
        {
            remove = 1;
        }
        if ((grid->p_to_remove[wallnodes[0]] == -1)
            || (grid->p_to_remove[wallnodes[1]] == -1)
            || (grid->p_to_remove[wallnodes[2]] == -1)
            || (grid->p_to_remove[wallnodes[3]] == -1))
        {
            //fprintf(stderr,"wall list uses removed node on a place where we have a node bc. Shouldn't happen.\n");
        }
        // remove if wall fully consists of bcnodes
        if ((isbcnode[wallnodes[0]]) && (isbcnode[wallnodes[1]])
            && (isbcnode[wallnodes[2]]) && (isbcnode[wallnodes[3]]))
        {
            // remove it!
            remove = 1;
        }
        if (grid->bcwallvol->list[i] == -1)
        {
            // remove it!
            remove = 1;
        }
        if (grid->new_elem[grid->bcwallvol->list[i]] == -1)
        {
            remove = 1;
        }
        if (remove == 1)
        {
            wallsremoved++;
        }
        else
        {
            // take it!
            grid->bcwall->list[4 * (i - wallsremoved) + 0] = grid->new_node[wallnodes[0]];
            grid->bcwall->list[4 * (i - wallsremoved) + 1] = grid->new_node[wallnodes[1]];
            grid->bcwall->list[4 * (i - wallsremoved) + 2] = grid->new_node[wallnodes[2]];
            grid->bcwall->list[4 * (i - wallsremoved) + 3] = grid->new_node[wallnodes[3]];
            grid->bcwallvol->list[i - wallsremoved] = grid->new_elem[grid->bcwallvol->list[i]];
        }
    }
    grid->bcwall->num -= 4 * wallsremoved;
    grid->bcwallvol->num -= wallsremoved;
    grid->bcwallpol->num -= wallsremoved;

    // 9. vertex replacement in bc inlet vertex list
    nremoved = 0;
    offset = 0;
    int bcelem[4];
    for (i = 0; i < grid->bcinvol.size(); i++)
    {
        inner_removed = 0;
        outer_removed = 0;

        bcelem[0] = grid->bcin->list[4 * i];
        bcelem[1] = grid->bcin->list[4 * i + 1];
        bcelem[2] = grid->bcin->list[4 * i + 2];
        bcelem[3] = grid->bcin->list[4 * i + 3];

        // Vergleich ueber zugehoerige Element!!!
        // entfernen, falls beide zugehoerigen Volumenelement (innen und aussen)
        // entfernt wurden oder nicht existieren (= ausserhalb Geometrie)
        if (grid->bcinvol_outer->list[i] == -1)
            inner_removed = 1;
        else if (grid->e_to_remove[grid->bcinvol_outer->list[i]] == -1)
            inner_removed = 1;
        if (grid->e_to_remove[grid->bcinvol[i]] == -1)
            outer_removed = 1;

        if (inner_removed && outer_removed)
        {
            // bcelem uses nodes that have been removed! remove bc!
            remove = 1;
        }

        if (remove == 1)
        {
            // nicht uebernehmen!!
            nremoved++;
            remove = 0;
            offset++;
        }
        else
        {
            // uebernehmen!!
            grid->bcin->list[4 * i - 4 * offset] = grid->new_node[bcelem[0]];
            grid->bcin->list[4 * i - 4 * offset + 1] = grid->new_node[bcelem[1]];
            grid->bcin->list[4 * i - 4 * offset + 2] = grid->new_node[bcelem[2]];
            grid->bcin->list[4 * i - 4 * offset + 3] = grid->new_node[bcelem[3]];
            //fprintf(stderr,"bcelem, new vertices %d %d %d %d\n", grid->new_node[bcelem[0]],
            //										             grid->new_node[bcelem[1]],
            //                                                     grid->new_node[bcelem[2]],
            //                                                     grid->new_node[bcelem[3]]);
            grid->bcinvol[i - offset] = grid->new_elem[grid->bcinvol[i]];
        }
    }
    grid->bcin->num -= nremoved * 4;
    grid->bcinpol->num -= nremoved;
    grid->bcinvol.resize(grid->bcinvol.size() - nremoved);

    // 10. vertex replacement in bc outlet vertex list
    nremoved = 0;
    offset = 0;
    for (i = 0; i < grid->bcoutvol->num; i++)
    {
        inner_removed = 0;
        outer_removed = 0;

        bcelem[0] = grid->bcout->list[4 * i];
        bcelem[1] = grid->bcout->list[4 * i + 1];
        bcelem[2] = grid->bcout->list[4 * i + 2];
        bcelem[3] = grid->bcout->list[4 * i + 3];

        // Vergleich ueber zugehoerige Element!!!
        // entfernen, falls beide zugehoerigen Volumenelement (innen und aussen)
        // entfernt wurden oder nicht existieren (= ausserhalb Geometrie)
        if (grid->bcoutvol_outer->list[i] == -1)
            inner_removed = 1;
        else if (grid->e_to_remove[grid->bcoutvol_outer->list[i]] == -1)
            inner_removed = 1;
        if (grid->e_to_remove[grid->bcoutvol->list[i]] == -1)
            outer_removed = 1;

        if (inner_removed && outer_removed)
        {
            // bcelem uses nodes that have been removed! remove bc!
            remove = 1;
        }

        if (remove == 1)
        {
            // nicht uebernehmen!!
            nremoved++;
            remove = 0;
            offset++;
        }
        else
        {
            // uebernehmen!!
            grid->bcout->list[4 * i - 4 * offset] = grid->new_node[bcelem[0]];
            grid->bcout->list[4 * i - 4 * offset + 1] = grid->new_node[bcelem[1]];
            grid->bcout->list[4 * i - 4 * offset + 2] = grid->new_node[bcelem[2]];
            grid->bcout->list[4 * i - 4 * offset + 3] = grid->new_node[bcelem[3]];
            //fprintf(stderr,"bcelem, new vertices %d %d %d %d\n", grid->new_node[bcelem[0]],
            //										             grid->new_node[bcelem[1]],
            //                                                     grid->new_node[bcelem[2]],
            //                                                     grid->new_node[bcelem[3]]);
            grid->bcoutvol->list[i - offset] = grid->new_elem[grid->bcoutvol->list[i]];
        }
    }
    grid->bcout->num -= nremoved * 4;
    grid->bcoutpol->num -= nremoved;
    grid->bcoutvol->num -= nremoved;

    /*
	 // 11. element replacement in bc wall volume list
	 offset=0;
	 for (i=0;i<grid->bcwallvol->num+wallsremoved;i++)
	 {
	 if (grid->new_elem[grid->bcwallvol->list[i]]==-1)
	 {
	 // was removed!
	 offset++;
	 }
	 else
	 {
	 // take it!
	 grid->bcwallvol->list[i-offset]=grid->new_elem[grid->bcwallvol->list[i]];
	 }
	 }
	 */

    // inlet node bcs
    int *inlet_node_type = new int[grid->p->nump];
    memset(inlet_node_type, -1, grid->p->nump * sizeof(int));

    for (i = 0; i < grid->bcinvol.size(); i++)
    {
        // grid->bcin_type value:  100-129: racks  150-155: inlet floor squares
        inlet_node_type[grid->bcin->list[4 * i + 0]] = grid->bcin_type[i];
        inlet_node_type[grid->bcin->list[4 * i + 1]] = grid->bcin_type[i];
        inlet_node_type[grid->bcin->list[4 * i + 2]] = grid->bcin_type[i];
        inlet_node_type[grid->bcin->list[4 * i + 3]] = grid->bcin_type[i];
    }

    int bcnode;

    // Node-bc inlet (outer geometry)
    k = 0;

    float l = 1.; // characteristic length
    float vin_abs = sqrt(
        pow(model->bcin_velo[0], 2) + pow(model->bcin_velo[1], 2)
        + pow(model->bcin_velo[2], 2));
    float _k = 3.75f * 0.001f * pow(vin_abs, 2);
    float _eps = 9.40f * 0.0001f * pow(fabs(vin_abs / l), 3);

    float vx, vy, vz;

    int racknr;
    float Q; // flow rate
    float A; // inlet area

    for (i = 0; i < grid->p->nump; i++)
    {
        vx = 0.;
        vy = 0.;
        vz = 0.;

        if (inlet_node_type[i] >= 0)
        {
            bcnode = i;

            if ((inlet_node_type[i] >= 100) && (inlet_node_type[i] < 130)) // this is a rack!
            {
                Q = model->cubes[inlet_node_type[i] - 100]->Q;
                racknr = inlet_node_type[i] - 100;

                if (model->cubes[racknr]->bc_type_plusx
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[1]
                        * model->cubes[inlet_node_type[i] - 100]->size[2];
                    vx = -Q / A;
                }
                if (model->cubes[racknr]->bc_type_minusx
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[1]
                        * model->cubes[inlet_node_type[i] - 100]->size[2];
                    vx = Q / A;
                }
                if (model->cubes[racknr]->bc_type_plusy
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[0]
                        * model->cubes[inlet_node_type[i] - 100]->size[2];
                    vy = -Q / A;
                }
                if (model->cubes[racknr]->bc_type_minusy
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[0]
                        * model->cubes[inlet_node_type[i] - 100]->size[2];
                    vy = Q / A;
                }
                if (model->cubes[racknr]->bc_type_plusz
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[0]
                        * model->cubes[inlet_node_type[i] - 100]->size[1];
                    vz = -Q / A;
                }
                if (model->cubes[racknr]->bc_type_minusz
                    == INLET + inlet_node_type[i] - 100)
                {
                    A = model->cubes[inlet_node_type[i] - 100]->size[0]
                        * model->cubes[inlet_node_type[i] - 100]->size[1];
                    vz = Q / A;
                }
            }
            else // this is a floor square
            {
                if ((inlet_node_type[i] < 150) || (inlet_node_type[i] > 155))
                {
                    fprintf(stderr, "wrong value (%d) for inlet_node_type!\n",
                            inlet_node_type[i]);
                    return NULL;
                }

                switch (inlet_node_type[i] - 150)
                {
                case 0: // 4 holes
                    vz = model->vin_4holes;
                    break;

                case 1: // 1 hole
                    vz = model->vin_1hole;
                    break;

                case 2: // open
                    //vz = model->vin_open_sx9;
                    break;

                case 3: // lochblech
                    vz = model->vin_lochblech;
                    break;

                case 4: // 4 holes open
                    vz = model->vin_4holesopen;
                    break;

                case 5: // open
                    //vz = model->vin_open_nec_cluster;
                    break;
                }
            }

            vin_abs = sqrt(vx * vx + vy * vy + vz * vz);
            _k = 3.75f * 0.001f * vin_abs * vin_abs;
            _eps = 9.40f * 0.0001f * pow(fabs(vin_abs / l), 3);

            grid->bcin_nodes.push_back(bcnode);
            grid->bcin_velos.push_back(vx);
            grid->bcin_velos.push_back(vy);
            grid->bcin_velos.push_back(vz);
            grid->bcin_velos.push_back(_k);
            grid->bcin_velos.push_back(_eps);
        }
    }

    // 12. vertex replacement in node-bc lists (ven and airconditioning)
    // transform ven and air nodes to new notation
    /* not here in archflow */

    // 13. it happens that there are unremoved nodes at the intersection of objects
    //     we remove all nodes that don't appear in the element list
    int *rep_nodes = (int *)calloc(grid->npoi, sizeof(int));
    for (i = 0; i < grid->e->nume; i++)
    {
        for (j = 0; j < 8; j++)
        {
            rep_nodes[grid->e->e[i][j]]++;
        }
    }
    // nodes gives us the number of appearance in the element list of each node now
    offset = 0;
    int node;
    for (i = 0; i < grid->p->nump; i++)
    {
        if (rep_nodes[i] == 0)
        {
            rep_nodes[i] = -1;
            offset++;
        }
        else
        {
            rep_nodes[i] = i - offset;
        }
    }
    // nodes is -1 for nodes to remove, for all other nodes it gives the new number
    // replace node list now
    offset = 0;
    for (i = 0; i < grid->npoi; i++)
    {
        if (rep_nodes[i] == -1)
        {
            offset++;
        }
        else
        {
            grid->p->x[i - offset] = grid->p->x[i];
            grid->p->y[i - offset] = grid->p->y[i];
            grid->p->z[i - offset] = grid->p->z[i];
        }
    }
    grid->npoi -= offset;
    grid->p->nump -= offset;

    // we replace all lists containing nodenumbers now once again ...
    for (i = 0; i < grid->e->nume; i++)
    {
        for (j = 0; j < 8; j++)
        {
            node = grid->e->e[i][j];
            grid->e->e[i][j] = rep_nodes[node];
        }
    }
    for (i = 0; i < grid->bcin_nodes.size(); i++)
    {
        node = grid->bcin_nodes[i];
        grid->bcin_nodes[i] = rep_nodes[node];
    }
    for (i = 0; i < grid->bcwallvol->num; i++)
    {
        for (j = 0; j < 4; j++)
        {
            node = grid->bcwall->list[4 * i + j];
            grid->bcwall->list[4 * i + j] = rep_nodes[node];
        }
    }
    for (i = 0; i < grid->bcinvol.size(); i++)
    {
        for (j = 0; j < 4; j++)
        {
            node = grid->bcin->list[4 * i + j];
            grid->bcin->list[4 * i + j] = rep_nodes[node];
        }
    }
    for (i = 0; i < grid->bcoutvol->num; i++)
    {
        for (j = 0; j < 4; j++)
        {
            node = grid->bcout->list[4 * i + j];
            grid->bcout->list[4 * i + j] = rep_nodes[node];
        }
    }

    // realloc of grid->p arrays and grid->e
    // can be done

    // free mem
    delete[] grid -> new_node;
    free(grid->p_to_remove);
    free(grid->e_to_remove);
    free(rep_nodes);

    return grid;
}

struct covise_info *
CreateGeometry4Covise(struct rech_model *model)
{
    int i;

    struct covise_info *ci = NULL;
    int ipol;
    int ipoi;

    ipoi = 0;
    ipol = 0;

    if ((ci = AllocCoviseInfo()) != NULL)
    {
        // Create Polygons
        for (i = 0; i < model->nobjects; i++)
        {
            CreateCubePolygons4Covise(ci, model, i, ipoi, ipol);
        }
    }

    // AddFloorPolygon
    ipoi = ci->p->nump;
    ipol = ci->pol->num;
    /* archflow ...
	 AddPoint(ci->p, -0.5*model->size[0],
	 -0.5*model->size[1],
	 0.0);
	 AddPoint(ci->p, +0.5*model->size[0],
	 -0.5*model->size[1],
	 0.0);
	 AddPoint(ci->p, +0.5*model->size[0],
	 +0.5*model->size[1],
	 0.0);
	 AddPoint(ci->p, -0.5*model->size[0],
	 +0.5*model->size[1],
	 0.0);
	 */
    AddPoint(ci->p, 0.0, 0.0, 0.0);
    AddPoint(ci->p, model->size[0], 0.0, 0.0);
    AddPoint(ci->p, model->size[0], model->size[1], 0.0);
    AddPoint(ci->p, 0.0, model->size[1], 0.0);

    // first polygon
    Add2Ilist(ci->vx, ipoi);
    Add2Ilist(ci->vx, ipoi + 1);
    Add2Ilist(ci->vx, ipoi + 2);
    Add2Ilist(ci->vx, ipoi + 3);
    Add2Ilist(ci->pol, ipol * 4);

    return ci;
}

int read_string(char *buf, char **str, const char *separator)
// buf        ... Zeile / Quelle
// str        ... zu lesender String (Ergebnis)
// separator  ... Zeichenfolge, nach der str steht

{
    int pos, i;
    char buffer2[100];
    char *str_help;

    buf = strstr(buf, separator); // mit separator
    buf += sizeof(char) * strlen(separator); // ohne separator
    pos = 0;
    for (i = 0; i < (int)strlen(buf); i++) // Tabs etc. weg
    {
        if (!(/*( buf[i]==' ' ) || */ (buf[i] == '\t') || (buf[i] == '\n')))
        {
            buffer2[pos] = buf[i];
            pos++;
        }
    }
    buffer2[pos] = '\0';

    str_help = strdup(buffer2); // alloc memory and copy string
    *str = &str_help[0];

    return (0);
}

int read_int(char *buf, int *izahl, const char *separator)
{
    buf = strstr(buf, separator); // mit separator
    buf += sizeof(char) * strlen(separator); // ohne separator

    sscanf(buf, "%d ", izahl);

    return (0);
}

int read_double(char *buf, double *dzahl, const char *separator)
{
    buf = strstr(buf, separator); // mit separator
    buf += sizeof(char) * strlen(separator); // ohne separator

    sscanf(buf, "%lf", dzahl);

    return (0);
}

int CreateCubePolygons4Covise(struct covise_info *ci, struct rech_model *model,
                              int number, int ipoi, int ipol)
{

    ipoi = ci->p->nump;
    ipol = ci->pol->num * 4;

    float pos[3];
    float size[3];
    pos[0] = model->cubes[number]->pos[0];
    pos[1] = model->cubes[number]->pos[1];
    pos[2] = model->cubes[number]->pos[2];
    size[0] = model->cubes[number]->size[0];
    size[1] = model->cubes[number]->size[1];
    size[2] = model->cubes[number]->size[2];

    AddPoint(ci->p, pos[0], pos[1], pos[2]);
    AddPoint(ci->p, pos[0] + size[0], pos[1], pos[2]);
    AddPoint(ci->p, pos[0] + size[0], pos[1] + size[1], pos[2]);
    AddPoint(ci->p, pos[0], pos[1] + size[1], pos[2]);

    AddPoint(ci->p, pos[0], pos[1], pos[2] + size[2]);
    AddPoint(ci->p, pos[0] + size[0], pos[1], pos[2] + size[2]);
    AddPoint(ci->p, pos[0] + size[0], pos[1] + size[1], pos[2] + size[2]);
    AddPoint(ci->p, pos[0], pos[1] + size[1], pos[2] + size[2]);

    // first polygon
    Add2Ilist(ci->vx, ipoi);
    Add2Ilist(ci->vx, ipoi + 1);
    Add2Ilist(ci->vx, ipoi + 2);
    Add2Ilist(ci->vx, ipoi + 3);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    // second polygon
    Add2Ilist(ci->vx, ipoi);
    Add2Ilist(ci->vx, ipoi + 1);
    Add2Ilist(ci->vx, ipoi + 5);
    Add2Ilist(ci->vx, ipoi + 4);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    // third polygon
    Add2Ilist(ci->vx, ipoi + 1);
    Add2Ilist(ci->vx, ipoi + 2);
    Add2Ilist(ci->vx, ipoi + 6);
    Add2Ilist(ci->vx, ipoi + 5);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    // fourth polygon
    Add2Ilist(ci->vx, ipoi + 2);
    Add2Ilist(ci->vx, ipoi + 3);
    Add2Ilist(ci->vx, ipoi + 7);
    Add2Ilist(ci->vx, ipoi + 6);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    // fifth polygon
    Add2Ilist(ci->vx, ipoi + 3);
    Add2Ilist(ci->vx, ipoi);
    Add2Ilist(ci->vx, ipoi + 4);
    Add2Ilist(ci->vx, ipoi + 7);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    // sixth polygon
    Add2Ilist(ci->vx, ipoi + 4);
    Add2Ilist(ci->vx, ipoi + 5);
    Add2Ilist(ci->vx, ipoi + 6);
    Add2Ilist(ci->vx, ipoi + 7);
    Add2Ilist(ci->pol, ipol);
    ipol += 4;

    return (0);
}

int subtractCube(struct rechgrid *grid, struct rech_model *model, int number)
{
    // subtracts cube from grid (removes cells and all interior nodes)
    // adapts coordinates of margins
    // and removes boundary conditions, if there are any
    //float spacing = model->spacing;
    float pos[3];
    pos[0] = model->cubes[number]->pos[0];
    pos[1] = model->cubes[number]->pos[1];
    pos[2] = model->cubes[number]->pos[2];
    float size[3];
    size[0] = model->cubes[number]->size[0];
    size[1] = model->cubes[number]->size[1];
    size[2] = model->cubes[number]->size[2];

    //fprintf(stderr,"subtractCube Nr. %d, ", number);

    // from cell counters to node counters
    int npoi_x = grid->nelem_x + 1;
    int npoi_y = grid->nelem_y + 1;
    int npoi_z = grid->nelem_z + 1;

    // counters
    int i, j, k;
    // i .. z-direction
    // j .. y-direction
    // k .. x-direction

    // coordinate range values min / max
    float xmin, xmax, ymin, ymax, zmin, zmax;

    // grid counter values min / max
    int ilo, ihi, jlo, jhi, klo, khi;

    //float bxmin = -0.5*model->size[0];
    float bxmin = 0.0;
    //float bymin = -0.5*model->size[1];
    float bymin = 0.0;
    float bzmin = 0.0;

    /*
	 float bxmax = 0.5*model->size[0];
	 float bymax = 0.5*model->size[1];
	 float bzmax = model->size[2];
	 */

    // all elements must be removed that lie completely inside the cube to be removed
    /* archflow
	 xmin=pos[0]-0.5*size[0];
	 xmax=pos[0]+0.5*size[0];
	 ymin=pos[1]-0.5*size[1];
	 ymax=pos[1]+0.5*size[1];
	 zmin=pos[2]-0.5*size[2];
	 zmax=pos[2]+0.5*size[2];
	 */
    xmin = pos[0];
    xmax = pos[0] + size[0];
    ymin = pos[1];
    ymax = pos[1] + size[1];
    zmin = pos[2];
    zmax = pos[2] + size[2];

    // ilo,ihi: node nrs, but highest value grid->npoi_(x,y,z)-1!!!
    ilo = (int)ceil(zmin / grid->spacing_z);
    ihi = (int)floor(zmax / grid->spacing_z);
    jlo = (int)ceil(ymin / grid->spacing_y);
    jhi = (int)floor(ymax / grid->spacing_y);
    klo = (int)ceil(xmin / grid->spacing_x);
    khi = (int)floor(xmax / grid->spacing_x);
    if (ilo <= model->ilo)
    {
        ihi = model->ilo + (ihi - ilo);
        ilo = model->ilo;
        zmin = grid->spacing_x * ilo;
        zmax = grid->spacing_x * ihi;
    }
    if (jlo <= model->jlo)
    {
        jhi = model->jlo + (jhi - jlo);
        jlo = model->jlo;
        ymin = grid->spacing_y * jlo;
        ymax = grid->spacing_y * jhi;
    }
    if (klo <= model->klo)
    {
        khi = model->klo + (khi - klo);
        klo = model->klo;
        xmin = grid->spacing_x * klo;
        xmax = grid->spacing_x * khi;
    }

    if (ihi >= model->ihi)
    {
        ilo = model->ihi - (ihi - ilo);
        ihi = model->ihi;
        zmin = grid->spacing_x * ilo;
        zmax = grid->spacing_x * ihi;
    }
    if (jhi >= model->jhi)
    {
        jlo = model->jhi - (jhi - jlo);
        jhi = model->jhi;
        ymin = grid->spacing_y * jlo;
        ymax = grid->spacing_y * jhi;
    }
    if (khi >= model->khi)
    {
        klo = model->khi - (khi - klo);
        khi = model->khi;
        xmin = grid->spacing_x * klo;
        xmax = grid->spacing_x * khi;
    }

    fprintf(stderr, "number=%2d\n", number);
    /*    fprintf(stderr,"imin=%2d imax=%2d jmin=%2d jmax=%2d kmin=%2d kmax=%2d\n",model->ilo,model->ihi,model->jlo,model->jhi,model->klo,model->khi);
	 fprintf(stderr,"ilo =%2d ihi =%2d jlo =%2d jhi =%2d klo =%2d khi =%2d\n",ilo,ihi,jlo,jhi,klo,khi);
	 fprintf(stderr,"npoi_x=%2d, npoi_y=%2d, npoi_z=%2d\n", npoi_x, npoi_y, npoi_z);
	 */

    /*
	 float khiexakt=(xmax+0.5*model->size[0])/grid->spacing_x;
	 fprintf(stderr,"khi=%d, khiexakt=%8.4f\n", khi, khiexakt);
	 float dx=((khi-khiexakt));
	 if ( (dx>0.) && (dx<0.1*grid->spacing_x) )
	 //khi++;
	 */

    // check if complete cube lies outside grid area
    if ((ilo < 0) && (ihi < 0))
    {
        fprintf(stderr,
                "\nz-position to small. cube outside volume, can't be removed\n");
        return -1;
    }
    if ((ilo > grid->nelem_z) && (ihi > grid->nelem_z))
    {
        fprintf(stderr,
                "\nz-position to small. cube outside volume, can't be removed\n");
        return -1;
    }
    if ((jlo < 0) && (jhi < 0))
    {
        fprintf(stderr,
                "\ny-position to small. cube outside volume, can't be removed\n");
        return -1;
    }
    if ((jlo > grid->nelem_y) && (jhi > grid->nelem_y))
    {
        fprintf(stderr,
                "\ny-position to small. cube outside volume, can't be removed\n");
        return -1;
    }
    if ((klo < 0) && (khi < 0))
    {
        fprintf(stderr,
                "\nx-position to small. cube outside volume, can't be removed\n");
        return -1;
    }
    if ((klo > grid->nelem_x) && (khi > grid->nelem_x))
    {
        fprintf(stderr,
                "\nx-position to small. cube outside volume, can't be removed\n");
        return -1;
    }

    // test
    // check if part of cube lies outside grid area
    if ((ilo < 0) && (ihi >= 0) && (ihi <= npoi_z))
        ilo = 0;
    if ((ihi > npoi_z) && (ilo >= 0) && (ilo <= npoi_z))
        ihi = npoi_z;
    if ((jlo < 0) && (jhi >= 0) && (jhi <= npoi_y))
        jlo = 0;
    if ((jhi > npoi_y) && (jlo >= 0) && (jlo <= npoi_y))
        jhi = npoi_y;
    if ((klo < 0) && (khi >= 0) && (khi <= npoi_x))
        klo = 0;
    if ((khi > npoi_x) && (klo >= 0) && (klo <= npoi_x))
        khi = npoi_x;

    int dont_ilo = 0;
    int dont_ihi = 0;
    int dont_jlo = 0;
    int dont_jhi = 0;
    int dont_klo = 0;
    int dont_khi = 0;

    // test mit <=, >=
    // if cube lies outside ...
    if (ilo <= model->ilo)
    {
        ilo = model->ilo; //=0
        zmin = ilo * grid->spacing_z;
        dont_ilo = 1;
        //fprintf(stderr,"dont_ilo==1!!!\n");
    }
    if (jlo <= model->jlo)
    {
        jlo = model->jlo; //=0
        ymin = jlo * grid->spacing_y;
        dont_jlo = 1;
        //fprintf(stderr,"dont_jlo==1!!!\n");
    }
    if (klo <= model->klo)
    {
        klo = model->klo; // =0
        xmin = klo * grid->spacing_x;
        dont_jlo = 1;
        //fprintf(stderr,"dont_klo==1!!!\n");
    }
    if (ihi >= model->ihi)
    {
        ihi = model->ihi; // =npoi_z
        zmax = ihi * grid->spacing_z;
        dont_ihi = 1;
        //fprintf(stderr,"dont_ihi==1!!!\n");
    }
    if (jhi >= model->jhi)
    {
        jhi = model->jhi; // =npoi_y
        ymax = jhi * grid->spacing_y;
        dont_jhi = 1;
        //fprintf(stderr,"dont_jhi==1!!!\n");
    }
    if (khi >= model->khi)
    {
        khi = model->khi; //=npoi_x
        xmax = khi * grid->spacing_x;
        dont_khi = 1;
        //fprintf(stderr,"dont_khi==1!!!\n");
    }

    // check if high and low values fall together
    // two reasons: a) coarse spacing / small objects
    // 				b) object just touches volume at margin

    // ATTENTION! HANDLE THIS WITH CARE.
    /*
	 int smalldx=0;
	 int smalldy=0;
	 int smalldz=0;

	 if (ihi==ilo)
	 {
	 if (ilo*grid->spacing_z-zmin < zmax-ihi*grid->spacing_z)
	 ilo--;
	 else
	 ihi++;
	 smalldz=1;

	 //    	fprintf(stderr,"z-expansion too small or object just touching volume, can't be removed!\n");
	 //        return -1;

	 }

	 if (jhi==jlo)
	 {
	 if (jlo*grid->spacing_y-ymin < ymax-jhi*grid->spacing_y)
	 jlo--;
	 else
	 jhi++;
	 smalldy=1;

	 //        fprintf(stderr,"y-expansion too small or object just touching volume, can't be removed!\n");
	 //        return -1;

	 }

	 if (khi==klo)
	 {
	 if (klo*grid->spacing_x-xmin < xmax-khi*grid->spacing_x)
	 klo--;
	 else
	 //khi++;
	 smalldx=1;

	 //        fprintf(stderr,"x-expansion too small or object just touching volume, can't be removed!\n");
	 //        return -1;

	 }
	 */
    // check that no index is smaller than zero or bigger than possible (imin/npoints_x/jmin/npoints_y/kmin/npoints_y)
    if ((ilo < 0) || (ilo > grid->nelem_z) || (ihi < 0) || (ihi > grid->nelem_z)
        || (jlo < 0) || (jlo > grid->nelem_y) || (jhi < 0)
        || (jhi > grid->nelem_y) || (klo < 0) || (klo > grid->nelem_x)
        || (khi < 0) || (khi > grid->nelem_x))
    {
        fprintf(stderr, "Error in subtractCube!");
    }

    // adapt coords of cube margins so that grid fits to real cube margins
    // real margins of cube: xmin-xmax, ymin-ymax, zmin-zmax
    // margins (completely inside cube): ilo-ihi, jlo-jhi, klo-khi
    // grid->spacing_x, grid->spacing_y, grid->spacing_z
    float dzmin = ilo * grid->spacing_z - zmin;
    float dzmax = zmax - (ihi * grid->spacing_z);

    /* archflow
	 float dymin = jlo*grid->spacing_y - (ymin+0.5*model->size[1]);
	 float dymax = ymax+0.5*model->size[1] - (jhi*grid->spacing_y);

	 float dxmin = klo*grid->spacing_x - (xmin+0.5*model->size[0]);
	 float dxmax = xmax+0.5*model->size[0] - (khi*grid->spacing_x);
	 */
    float dymin = jlo * grid->spacing_y - ymin;
    float dymax = ymax - (jhi * grid->spacing_y);

    float dxmin = klo * grid->spacing_x - xmin;
    float dxmax = xmax - (khi * grid->spacing_x);

    if (dzmin < 0)
        fprintf(stderr, "\nerror in subtractCube! dzmin<0\n");
    if (dzmax < 0)
        fprintf(stderr, "\nerror in subtractCube! dzmax<0\n");

    if (dymin < 0)
        fprintf(stderr, "\nerror in subtractCube! dymin<0\n");
    if (dymax < 0)
        fprintf(stderr, "\nerror in subtractCube! dymax<0\n");

    if (dxmin < 0)
        fprintf(stderr, "\nerror in subtractCube! dxmin<0\n");
    if (dxmax < 0)
        fprintf(stderr, "\nerror in subtractCube! dxmax<0\n");

    float tol = 0.4f;
    // two possibilities:
    // 1. make cells outside larger
    // 2. make cells outside smaller
    // tol < 0.5: cells outside get larger
    // tol > 0.5: cells outside get smaller: quad with angles > 90 degrees possible!

    if ((dzmin > tol * grid->spacing_z) && (dont_ilo == 0))
    {
        // move ilo-1 upwards
        ilo--;
    }
    if ((dzmax > tol * grid->spacing_z) && (dont_ihi == 0))
    {
        // move ihi+1 downwards
        ihi++;
    }

    if ((dymin > tol * grid->spacing_y) && (dont_jlo == 0))
    {
        // move jlo-1 backwards
        jlo--;
    }
    //&& (jhi<model->jhi) )
    if ((dymax > tol * grid->spacing_y) && (dont_jhi == 0))
    {
        // move jhi+1 frontwards
        jhi++;
    }

    if ((dxmin > tol * grid->spacing_x) && (dont_klo == 0))
    {
        // move klo-1 right
        klo--;
    }
    if ((dxmax > tol * grid->spacing_x) && (dont_khi == 0))
    {
        // move khi+1 left
        khi++;
    }

    // now move it ...
    if (dont_ilo == 0)
    {
        if (dzmin <= tol * grid->spacing_z)
        {
            // move ilo dz downwards
            i = ilo;
            for (j = jlo; j <= jhi; j++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->z[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=dzmin;
                    grid->p->z[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bzmin
                                                                           + i * grid->spacing_z - dzmin;
                }
            }
        }
        else
        {
            // move ilo dz upwards
            i = ilo;
            for (j = jlo; j <= jhi; j++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->z[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=grid->spacing_z-dzmin;
                    grid->p->z[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bzmin
                                                                           + i * grid->spacing_z + grid->spacing_z - dzmin;
                }
            }
        }
    }

    if (dont_jlo == 0)
    {
        if (dymin <= tol * grid->spacing_y)
        {
            // move jlo frontwards
            j = jlo;
            for (i = ilo; i <= ihi; i++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->y[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=dymin;
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bymin
                                                                           + j * grid->spacing_y - dymin;
                }
            }
        }
        else
        {
            // move jlo dy backwards
            j = jlo;
            for (i = ilo; i <= ihi; i++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->y[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=grid->spacing_y-dymin;
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bymin
                                                                           + j * grid->spacing_y + grid->spacing_y - dymin;
                }
            }
        }
    }

    if (dont_klo == 0)
    {
        if (dxmin <= tol * grid->spacing_x)
        {
            // move klo dx left
            k = klo;
            for (i = ilo; i <= ihi; i++)
            {
                for (j = jlo; j <= jhi; j++)
                {
                    //grid->p->x[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=dxmin;
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bxmin
                                                                           + k * grid->spacing_x - dxmin;
                }
            }
        }
        else
        {
            // move klo dx right
            k = klo;
            for (i = ilo; i <= ihi; i++)
            {
                for (j = jlo; j <= jhi; j++)
                {
                    //grid->p->x[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=grid->spacing_x-dxmin;
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bxmin
                                                                           + k * grid->spacing_x + grid->spacing_x - dxmin;
                }
            }
        }
    }

    if (dont_ihi == 0)
    {
        if (dzmax <= tol * grid->spacing_z)
        {
            // move ihi dz upwards
            i = ihi;
            for (j = jlo; j <= jhi; j++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->z[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dzmax;
                    grid->p->z[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bzmin
                                                                           + i * grid->spacing_z + dzmax;
                }
            }
        }
        else
        {
            // move ihi dz downwards
            i = ihi;
            for (j = jlo; j <= jhi; j++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->z[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=grid->spacing_z-dzmax;
                    grid->p->z[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bzmin
                                                                           + i * grid->spacing_z - (grid->spacing_z - dzmax);
                }
            }
        }
    }

    if (dont_jhi == 0)
    {
        if (dymax <= tol * grid->spacing_y)
        {
            // move jhi backwards
            j = jhi;
            for (i = ilo; i <= ihi; i++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->y[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dymax;
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bymin
                                                                           + j * grid->spacing_y + dymax;
                }
            }
        }
        else
        {
            // move jhi dy frontwards
            j = jhi;
            for (i = ilo; i <= ihi; i++)
            {
                for (k = klo; k <= khi; k++)
                {
                    //grid->p->y[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=grid->spacing_y-dymax;
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bymin
                                                                           + j * grid->spacing_y - (grid->spacing_y - dymax);
                }
            }
        }
    }

    if (dont_khi == 0)
    {
        if (dxmax <= tol * grid->spacing_x)
        {
            // move khi dx right
            k = khi;
            for (i = ilo; i <= ihi; i++)
            {
                for (j = jlo; j <= jhi; j++)
                {
                    //grid->p->x[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dxmax;
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bxmin
                                                                           + k * grid->spacing_x + dxmax;
                }
            }
        }
        else
        {
            // move khi dx left
            k = khi;
            for (i = ilo; i <= ihi; i++)
            {
                for (j = jlo; j <= jhi; j++)
                {
                    //grid->p->x[i*(npoi_x*npoi_y)+j*(npoi_x)+k]-=grid->spacing_x-dxmax;
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = bxmin
                                                                           + k * grid->spacing_x - (grid->spacing_x - dxmax);
                }
            }
        }
    }

    /*
	 // first try (is not very good, we have to do it with a bit more effort ...)
	 for (i=ilo;i<=ihi;i++)	// z-dir.
	 {
	 for (j=jlo;j<=jhi;j++) 	// y-dir.
	 {
	 for (k=klo;k<=khi;k++)  // x-dir.
	 {
	 if ( (i=ilo) || (i=ihi) || (j=jlo) || (j=jhi) || (k=klo) || (k=khi) ) // margin!!
	 {
	 dx=dxmin+(k-klo)*(dxmax-dxmin)/(khi-klo);
	 dy=dymin+(j-jlo)*(dymax-dymin)/(jhi-jlo);
	 dz=dzmin+(i-ilo)*(dzmax-dzmin)/(ihi-ilo);

	 grid->p->x[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dx;
	 grid->p->y[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dy;
	 grid->p->z[i*(npoi_x*npoi_y)+j*(npoi_x)+k]+=dz;
	 }
	 }
	 }
	 }
	 */

    // now: remove it!
    // we have to remove all nodes with index
    // ilo <= i <= ihi
    // jlo <= j <= jhi
    // klo <= k <= khi
    // and the corresponding elements
    int removed_nodes = 0;

    // in case our cube to subtract lies directly at the margin, we also have to subtract the outer nodes!
    int istart, iend, jstart, jend, kstart, kend;

    if (number != -1)
    {
        if (ilo == 0)
            istart = 0;
        else
            istart = ilo + 1;
        if (ihi == grid->nelem_z)
            iend = npoi_z;
        else
            iend = ihi;

        if (jlo == 0)
            jstart = 0;
        else
            jstart = jlo + 1;
        if (jhi == grid->nelem_y)
            jend = npoi_y;
        else
            jend = jhi;

        if (klo == 0)
            kstart = 0;
        else
            kstart = klo + 1;
        if (khi == grid->nelem_x)
            kend = npoi_x;
        else
            kend = khi;
        /*
		 }
		 else
		 {
		 istart=ilo+1;
		 iend=ihi;
		 jstart=jlo+1;
		 jend=jhi;
		 kstart=klo+1;
		 kend=khi;
		 }
		 */
        for (i = istart; i < iend; i++) // z-dir.
        {
            for (j = jstart; j < jend; j++) // y-dir.
            {
                for (k = kstart; k < kend; k++) // x-dir.
                {
                    grid->p_to_remove[i * (npoi_x * npoi_y) + j * (npoi_x) + k] = -1;
                    removed_nodes++;
                }
            }
        }
        fprintf(stderr, "\tremoved %d nodes and ", removed_nodes);

        int removed_elements = 0;
        for (i = ilo; i < ihi; i++) // z-dir.
        {
            for (j = jlo; j < jhi; j++) // y-dir.
            {
                for (k = klo; k < khi; k++) // x-dir.
                {
                    grid->e_to_remove[i * (grid->nelem_x * grid->nelem_y)
                                      + j * (grid->nelem_x) + k] = -1;
                    //fprintf(stderr,"removed element nr. %d!\n",i*(grid->nelem_x*grid->nelem_y)+j*(grid->nelem_x)+k);
                    removed_elements++;
                }
            }
        }
        fprintf(stderr, "%d elements!\n", removed_elements);
    }

    model->cubes[number]->ilo = ilo;
    model->cubes[number]->ihi = ihi;
    model->cubes[number]->jlo = jlo;
    model->cubes[number]->jhi = jhi;
    model->cubes[number]->klo = klo;
    model->cubes[number]->khi = khi;

    // do we need to change existing bouundary conditions?

    // change bcs

    //	fprintf(stderr, "ilo =%2d ihi =%2d jlo =%2d jhi =%2d klo =%2d khi =%2d\n",
    //			ilo, ihi, jlo, jhi, klo, khi);

    return 0;
}

int DefineBCs(struct rechgrid *grid, struct rech_model *model, int number)
{
    /*
	 int npoi_x = grid->npoi_x;
	 int npoi_y = grid->npoi_y;
	 int npoi_z = grid->npoi_z;
	 */
    int nelem_x = grid->nelem_x;
    int nelem_y = grid->nelem_y;
    int nelem_z = grid->nelem_z;
    int ilo = 0, ihi = 0, jlo = 0, jhi = 0, klo = 0, khi = 0;
    if (number == -1)
    {
        model->ilo = 0;
        model->ihi = nelem_z;
        model->jlo = 0;
        model->jhi = nelem_y;
        model->klo = 0;
        model->khi = nelem_x;

        model->bc_type_minusy = WALL;
        model->bc_type_plusy = WALL;
        model->bc_type_minusz = WALL;
        model->bc_type_plusz = WALL; // hier muss aber noch eine Knoten-RB dazu (Klimaanlage)!
        model->bc_type_minusx = WALL;
        model->bc_type_plusx = WALL;

        ilo = model->ilo;
        ihi = model->ihi;
        jlo = model->jlo;
        jhi = model->jhi;
        klo = model->klo;
        khi = model->khi;
    }
    else
    {
        // define bc types
        //	switch (number) {
        //	case -1:
        // this is the complete outside geometry!
        // walls -y ,+y, -z, +z
        // ilo,ihi,...: elements!

        //		break;

        ilo = model->cubes[number]->ilo;
        ihi = model->cubes[number]->ihi;
        jlo = model->cubes[number]->jlo;
        jhi = model->cubes[number]->jhi;
        klo = model->cubes[number]->klo;
        khi = model->cubes[number]->khi;

        switch (model->cubes[number]->direction)
        {
        //	case 0: // Infrastruktur
        //	case 1: // Phoenix
        //	case 2: // Strider
        //	case 4: // Disk Rack 1
        //	case 5: // Disk Rack 2
        //	case 6: // Disk Rack 3
        //	case 9: // IOX
        //	case 10: // Netz2
        //	case 11: // FC-Netz

        case BC_MINUS_Y:
            // Belueftung in negativer y-Richtung

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )

            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = WALL;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = WALL;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = OUTLET + number;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = INLET + number;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = WALL;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = WALL;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_PLUS_Z:
            // Blech(SX8R)

            // Belueftung von unten nach oben

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = INLET + number;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = OUTLET + number;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = WALL;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = WALL;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = WALL;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = WALL;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_PLUS_X:
            //Netz1

            // Belueftung in positiver x-Richtung

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = WALL;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = WALL;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = WALL;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = WALL;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = INLET + number;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = OUTLET + number;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_PLUS_Y:
            // Disk6

            // Belueftung in positiver y-Richtung

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = WALL;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = WALL;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = INLET + number;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = OUTLET + number;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = WALL;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = WALL;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_MINUS_X:
            // kommt im Moment nirgends vor

            // Belueftung in negativer x-Richtung

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = WALL;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = WALL;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = WALL;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = WALL;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = OUTLET + number;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = INLET + number;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_MINUS_Z:

            // Belueftung in negativer z-Richtung

            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = OUTLET + number;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = INLET + number;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = WALL;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = WALL;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = WALL;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = WALL;
            else
                model->cubes[number]->bc_type_plusx = 0;

            break;

        case BC_NONE:
            // z-direction
            // if ( (inside) or (outside and not normal outside wall) )
            if ((ilo != 0) || ((ilo == 0) && (model->bc_type_minusz != WALL)))
                model->cubes[number]->bc_type_minusz = WALL;
            else
                model->cubes[number]->bc_type_minusz = 0;
            if ((ihi != grid->npoi_z)
                || ((ihi == grid->npoi_z) && (model->bc_type_plusz != WALL)))
                model->cubes[number]->bc_type_plusz = WALL;
            else
                model->cubes[number]->bc_type_plusz = 0;

            // y-direction
            if ((jlo != 0) || ((jlo == 0) && (model->bc_type_minusy != WALL)))
                model->cubes[number]->bc_type_minusy = WALL;
            else
                model->cubes[number]->bc_type_minusy = 0;
            if ((jhi != grid->npoi_y)
                || ((jhi == grid->npoi_y) && (model->bc_type_plusy != WALL)))
                model->cubes[number]->bc_type_plusy = WALL;
            else
                model->cubes[number]->bc_type_plusy = 0;

            // x-direction
            if ((klo != 0) || ((klo == 0) && (model->bc_type_minusx != WALL)))
                model->cubes[number]->bc_type_minusx = WALL;
            else
                model->cubes[number]->bc_type_minusx = 0;
            if ((khi != grid->npoi_x)
                || ((khi == grid->npoi_x) && (model->bc_type_plusx != WALL)))
                model->cubes[number]->bc_type_plusx = WALL;
            else
                model->cubes[number]->bc_type_plusx = 0;

            fprintf(stderr, "Warning: cubus no %d treated as Wall!\n", number);
            break;

        default:
            fprintf(stderr,
                    "Error, invalid direction type specified for cubus no %d !",
                    number);
        }
    }

    // walls: take care that there are no double walls!
    // ilo,ihi,...:
    //	if (number != -1) {
    //		ilo = model->cubes[number]->ilo;
    //		ihi = model->cubes[number]->ihi;
    //		jlo = model->cubes[number]->jlo;
    //		jhi = model->cubes[number]->jhi;
    //		klo = model->cubes[number]->klo;
    //		khi = model->cubes[number]->khi;
    //	}

    /*
	 // test
	 if (ihi>=grid->npoi_z)
	 model->cubes[number]->ihi=grid->npoi_z-1;
	 if (jhi>=grid->npoi_y)
	 model->cubes[number]->jhi=grid->npoi_y-1;
	 if (khi>=grid->npoi_x)
	 model->cubes[number]->khi=grid->npoi_x-1;
	 */

    // generate floor inlet and ceiling outlet BCs
    // ...
    // floor inlet: Schachbrett (Kacheln)
    // ceiling outlet
    // shift nodes directly here
    /*
	 if (number==0)
	 {
	 model->cubes[number]->bc_special=77;
	 }
	 else
	 {
	 model->cubes[number]->bc_special=0;
	 }
	 */

    return 0;
}

int GenerateFloorBCs(struct rechgrid *grid, struct rech_model *model)
{
    // read BC file
#if NONCOMPLIENTPARSING
    FILE* stream;
    char buf[2000];
    double b[6];
    double mat[16];
    char* token;
    int jhi, khi;
#endif
    float x, y;
    int i, j, k;

    int jlo; // y-position in structured grid
    int klo; // x-position


    //read from PLMXML-File
    plmxml_file = model->PLMXMLFile;
    document = myParser.parse(plmxml_file, document);

    // generate the BCs
    std::vector<int> xpos_4holes;
    std::vector<int> ypos_4holes;
    std::vector<int> xpos_4holesopen;
    std::vector<int> ypos_4holesopen;
    std::vector<int> xpos_1hole;
    std::vector<int> ypos_1hole;
    std::vector<int> xpos_open_sx9;
    std::vector<int> ypos_open_sx9;
    std::vector<int> xpos_lochblech;
    std::vector<int> ypos_lochblech;
    std::vector<int> xpos_open_nec_cluster;
    std::vector<int> ypos_open_nec_cluster;

    int spacing = (int)model->spacing;

    // we store our floor bcs in grid->floorbcs. This should be initialized to 0
    // floorbcs: 0=wall,
    //           1=IN_FLOOR_4HOLES,
    //           2=IN_FLOOR_1HOLE,
    //           3=IN_FLOOR_OPEN_SX9,
    //           4=IN_FLOOR_LOCHBLECH,
    //           5=IN_FLOOR_4HOLESOPEN
    //           6=IN_FLOOR_OPEN_NEC_CLUSTER
    grid->floorbcs = new int[grid->nelem_x * grid->nelem_y];
    memset(grid->floorbcs, 0, grid->nelem_x * grid->nelem_y * sizeof(int));

    vector<int> angle;

//  if ((stream = fopen(model->BCFile, "r")) == NULL)
//    {
//      fprintf(stderr, "Cannot open '%s'! Does file exist?\n", model->BCFile);
//      return -1;
//    }
//
//  fgets(buf, 2000, stream);
//
//  if (strncmp(buf, "IN_FLOOR_4HOLES", 15))   //Floor inlets type 1
//    {
//      fprintf(stderr,
//          "wrong file format (%s), must start with 'IN_FLOOR_4HOLES'\n",
//          model->BCFile);
//      return -1;
//    }
#if NONCOMPLIENTPARSING
    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_type1), 5))
        {
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];
                            //                          cout << "X:" << x << " Y:" << y << endl;
                            x -= 1; // we begin to count by 0 here
                            y -= 1;
                            x *= spacing;
                            y *= spacing;
                            if ((x >= grid->nelem_x) || (y >= grid->nelem_y)
                                || (x < 0) || (y < 0))
                            {
                                //fprintf(stderr,"specified boundary condition is out of domain and is not used\n");
                            }
                            else
                            {
                                xpos_4holes.push_back(x);
                                ypos_4holes.push_back(y);
                                for (j = 0; j < spacing; j++) // x-direction
                                {
                                    for (k = 0; k < spacing; k++) // y-direction
                                    {
                                        if ((y + k) * grid->nelem_x + (x + j)
                                            > grid->nelem_x
                                              * grid->nelem_y)
                                        {
                                            cerr << "Fehler!" << endl;
                                        }
                                        else
                                        {
                                            grid->floorbcs[(y + k)
                                                           * grid->nelem_x + (x + j)] = 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //  fgets(buf, 2000, stream);
    //  token = strtok(buf, " ");
    //  while ((token) && (token[0] != '\n'))
    //    {
    //      sscanf(token, "%d.%d", &x, &y);
    //fprintf(stderr,"x=%d, y=%d\n",x,y);

    //      token = strtok(NULL, " ");
    //    }
    model->n_floor_4holes = (int)xpos_4holes.size();
    fprintf(stderr, "Inlet floor squares with 4 holes: %d\n",
            model->n_floor_4holes);

    // empty line
    //  fgets(buf, 2000, stream);
    //  fgets(buf, 2000, stream);
    //  if (strncmp(buf, "IN_FLOOR_4HOLESOPEN", 19)) // Floor inlets type 2
    //    {
    //      fprintf(stderr,
    //          "wrong file format (%s), expected 'IN_FLOOR_4HOLESOPEN'\n",
    //          model->BCFile);
    //      return -1;
    //    }
    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_type2), 5))
        {
            //          for (int iii = 0; iii < element->getAttributes()->getLength(); iii++)
            //            cout
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeName())
            //                << " -> "
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeValue())
            //
            //                << endl;
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];
                            //                          cout << "X:" << x << " Y:" << y << endl;
                            x -= 1; // we begin to count by 0 here
                            y -= 1;
                            x *= (int)spacing;
                            y *= (int)spacing;
                            if ((x >= grid->nelem_x) || (y >= grid->nelem_y)
                                || (x < 0) || (y < 0))
                            {
                                //fprintf(stderr,"specified boundary condition is out of domain and is not used.\n");
                            }
                            else
                            {
                                xpos_4holesopen.push_back(x);
                                ypos_4holesopen.push_back(y);
                                for (j = 0; j < spacing; j++) // x-direction
                                {
                                    for (k = 0; k < spacing; k++) // y-direction
                                    {
                                        if ((y + k) * grid->nelem_x + (x + j)
                                            > grid->nelem_x
                                              * grid->nelem_y)
                                        {
                                            cerr << "Fehler!" << endl;
                                        }
                                        else
                                            grid->floorbcs[(y + k)
                                                           * grid->nelem_x + (x + j)] = 5;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //  fgets(buf, 2000, stream);
    //  token = strtok(buf, " ");
    //  while ((token) && (token[0] != '\n'))
    //    {
    //      sscanf(token, "%d.%d", &x, &y);
    //fprintf(stderr,"x=%d, y=%d\n",x,y);

    //      token = strtok(NULL, " ");
    //    }
    model->n_floor_4holesopen = (int)xpos_4holesopen.size();
    fprintf(stderr, "Inlet floor squares with 4 open holes: %d\n",
            model->n_floor_4holesopen);

    //  // empty line
    //  fgets(buf, 2000, stream);
    //
    //  fgets(buf, 2000, stream);
    //  if (strncmp(buf, "IN_FLOOR_1HOLE", 14))     // Floor inlets type 4
    //    {
    //      fprintf(stderr, "wrong file format (%s), expected 'IN_FLOOR_1HOLE'\n",
    //          model->BCFile);
    //      return -1;
    //    }
    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_type4), 5))
        {
            //          for (int iii = 0; iii < element->getAttributes()->getLength(); iii++)
            //            cout
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeName())
            //                << " -> "
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeValue())
            //
            //                << endl;
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];
                            //                          cout << "X:" << x << " Y:" << y << endl;
                            x -= 1; // we begin to count by 0 here
                            y -= 1;
                            x *= (int)spacing;
                            y *= (int)spacing;
                            if ((x >= grid->nelem_x) || (y >= grid->nelem_y)
                                || (x < 0) || (y < 0))
                            {
                                //fprintf(stderr,"specified boundary condition is out of domain and is not used.\n");
                            }
                            else
                            {
                                xpos_1hole.push_back(x);
                                ypos_1hole.push_back(y);
                                for (j = 0; j < spacing; j++) // x-direction
                                {
                                    for (k = 0; k < spacing; k++) // y-direction
                                    {
                                        if ((y + k) * grid->nelem_x + (x + j)
                                            > grid->nelem_x
                                              * grid->nelem_y)
                                        {
                                            cerr << "Fehler!" << endl;
                                        }
                                        else
                                            grid->floorbcs[(y + k)
                                                           * grid->nelem_x + (x + j)] = 2;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //  fgets(buf, 2000, stream);
    //  token = strtok(buf, " ");
    //  while ((token) && (token[0] != '\n'))
    //    {
    //      sscanf(token, "%d.%d", &x, &y);
    //fprintf(stderr,"x=%d, y=%d\n",x,y);

    //      token = strtok(NULL, " ");
    //    }
    model->n_floor_1hole = (int)xpos_1hole.size();
    fprintf(stderr, "Inlet floor squares with 1 hole: %d\n",
            model->n_floor_1hole);

    //  // empty line
    //  fgets(buf, 2000, stream);
    //
    //  fgets(buf, 2000, stream);
    //  if (strncmp(buf, "IN_FLOOR_LOCHBLECH", 18))     // Floor inlets type 3
    //    {
    //      fprintf(stderr, "wrong file format (%s), expected 'IN_FLOOR_LOCHBLECH'\n",
    //          model->BCFile);
    //      return -1;
    //    }
    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_type3), 5))
        {
            //          for (int iii = 0; iii < element->getAttributes()->getLength(); iii++)
            //            cout
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeName())
            //                << " -> "
            //                << XMLString::transcode(
            //                    element->getAttributes()->item(iii)->getNodeValue())
            //
            //                << endl;
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];
                            //                          cout << "X:" << x << " Y:" << y << endl;
                            x -= 1; // we begin to count by 0 here
                            y -= 1;
                            x *= (int)spacing;
                            y *= (int)spacing;
                            if ((x >= grid->nelem_x) || (y >= grid->nelem_y)
                                || (x < 0) || (y < 0))
                            {
                                //fprintf(stderr,"specified boundary condition is out of domain and is not used.\n");
                            }
                            else
                            {
                                xpos_lochblech.push_back(x);
                                ypos_lochblech.push_back(y);
                                for (j = 0; j < spacing; j++) // x-direction
                                {
                                    for (k = 0; k < spacing; k++) // y-direction
                                    {
                                        if ((y + k) * grid->nelem_x + (x + j)
                                            > grid->nelem_x
                                              * grid->nelem_y)
                                        {
                                            cerr << "Fehler!" << endl;
                                        }
                                        else
                                            grid->floorbcs[(y + k)
                                                           * grid->nelem_x + (x + j)] = 4;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#else
    listUserData = document->getElementsByTagName(TAG_UserData);
    listInst = document->getElementsByTagName(TAG_transform);
    listBounds = document->getElementsByTagName(TAG_Bound);
    for (i = 0; i < listUserData->getLength(); i++)
    {
        DOMElement *curEle = dynamic_cast<DOMElement *>(listUserData->item(i));
        DOMElement *curPar = dynamic_cast<DOMElement *>(curEle->getParentNode());
        double mat[16];
        double b[6];
        string parName = XMLString::transcode(curPar->getAttribute(TAG_name));
        if (curEle)
        {
            string curID;
            for (DOMNode *child = curEle->getFirstChild(); child != NULL;
                 child = child->getNextSibling())
            {
                DOMElement *curChild = dynamic_cast<DOMElement *>(child);

                if (curChild)
                {
                    if ((XMLString::compareString(
                             curChild->getAttribute(TAG_title), TAG_DEBBLevel)
                         == 0)
                        && (XMLString::compareString(
                                curChild->getAttribute(TAG_value),
                                TAG_Inlet) == 0)
                        && (strstr(parName.c_str(), "FlowPump") != NULL))
                    {
                        curID = XMLString::transcode(
                            curEle->getAttribute(TAG_id));
                        cerr
                            << XMLString::transcode(
                                   curChild->getAttribute(TAG_title))
                            << " "
                            << XMLString::transcode(
                                   curChild->getAttribute(TAG_value))
                            << endl;
                        break;
                    }
                }
            }
            if (curID.empty())
                continue;
            cerr << curID << " is an Inlet!" << endl;
            for (int ii = 0; ii < listInst->getLength(); ii++)
            {

                DOMElement *curInst = dynamic_cast<DOMElement *>(listInst->item(
                    ii));
                if (curInst)
                {
                    string idInst = XMLString::transcode(
                        curInst->getAttribute(TAG_id));
                    idInst.erase(0, 2);
                    if (strstr(curID.c_str(), idInst.c_str()) != NULL)
                    {
                        cerr << curID << " contains " << idInst << endl;
                        curID = idInst;
                        char *transString = XMLString::transcode(
                            curInst->getTextContent());
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];
                            if (mat[0] > 0) // angle=0
                                angle.push_back(0);
                            if (mat[1] < 0) // angle=90
                                angle.push_back(90);
                            if (mat[0] < 0) // angle=180
                                angle.push_back(180);
                            if (mat[1] > 0) // angle=270
                                angle.push_back(270);
                        }
                        break;
                    }
                }
            }
            DOMNode *curSib = curEle->getNextSibling();
            while (curSib != NULL)
            {
                if (curSib->getNodeType() == DOMNode::ELEMENT_NODE)
                {
                    DOMElement *bound = dynamic_cast<DOMElement *>(curSib);
                    if (XMLString::compareString(bound->getNodeName(),
                                                 TAG_Bound) == 0)
                    {
                        if (bound)
                        {
                            string boundID = XMLString::transcode(
                                bound->getAttribute(TAG_id));
                            if (strstr(boundID.c_str(), curID.c_str()) != NULL)
                            {
                                cerr << boundID << " also matches!" << endl;
                                char *boundString = XMLString::transcode(
                                    bound->getAttribute(TAG_values));
                                if (sscanf(boundString,
                                           "%lg %lg %lg %lg %lg %lg ", &b[0],
                                           &b[1], &b[2], &b[3], &b[4], &b[5]))
                                {
                                }
                                printf(
                                    "Inlet found at: (%lg,%lg,%lg) with size: (%lg,%lg,%lg)\n",
                                    mat[12], mat[13], mat[14], b[3], b[4],
                                    b[5]);
                                break;
                            }
                        }
                    }
                }
                curSib = curSib->getNextSibling();
            }
            switch (angle.back())
            {
            case 0:
                break;
            case 90:
                y -= 2.0 * b[3];
                std::swap(b[3], b[4]);
                break;
            case 180:
                x -= 2.0 * b[3];
                y -= 2.0 * b[4];
                break;
            case 270:
                x -= 2.0 * b[4];
                std::swap(b[3], b[4]);
                break;
            default:
                printf("faulty angle provided!");
            }
            if (b[3] == 0.3 && b[4] == 0.3)
            {
                x -= 1.; // we begin to count by 0 here
                y -= 1.;
                //				cout << x << "/" << y << endl;
                jlo = ((x + 1) * grid->nelem_x) / model->size[0];
                klo = ((y + 1) * grid->nelem_y) / model->size[1];
                //				cout << jlo << "/" << klo << endl;
                //				x *= spacing;
                //				y *= spacing;
                x = jlo;
                y = klo;
                if ((x >= grid->nelem_x) || (y >= grid->nelem_y) || (x < 0)
                    || (y < 0))
                {
                    fprintf(stderr,
                            "specified boundary condition is out of domain and is not used.\n x:%lg y:%lg",
                            x, y);
                }
                else
                {
                    //					cout << x << "/" << y << endl;
                    xpos_lochblech.push_back(x);
                    ypos_lochblech.push_back(y);
                    for (j = 0; j < spacing; j++) // x-direction
                    {
                        for (k = 0; k < spacing; k++) // y-direction
                        {
                            if ((y + k) * grid->nelem_x + (x + j)
                                > grid->nelem_x * grid->nelem_y)
                            {
                                cerr << "Fehler!" << endl;
                            }
                            else
                                grid->floorbcs[((int)y + k) * grid->nelem_x
                                               + ((int)x + j)] = 4;
                        }
                    }
                }
            }
            else
            {
                printf("Inlet size not supported yet!");
            }
        }
    }
#endif
    //	for (i = 0; i < (grid->nelem_x * grid->nelem_y); i++) {
    //		cerr << grid->floorbcs[i];
    //		if ((i + 1) % grid->nelem_x == 0) {
    //			cerr << endl;
    //		}
    //	}

    model->n_floor_lochblech = (int)xpos_lochblech.size();
    fprintf(stderr, "Inlet floor squares with lochblech: %d\n",
            model->n_floor_lochblech);

    // calculate area of floor inlet

    model->Ain_floor_4holes = (float)(4 * M_PI
                                      * (sqr(0.14 / 2) - sqr(0.134 / 2) + sqr(0.12 / 2) - sqr(0.114 / 2)
                                         + sqr(0.098 / 2) - sqr(0.092 / 2) + sqr(
                                                                                 0.075 / 2) - sqr(0.069 / 2)) * 0.33);
    model->Ain_floor_4holesopen = (float)(4 * M_PI * (sqr(0.15 / 2)));
    model->Ain_floor_1hole = (float)(M_PI * sqr(0.4 / 2));
    model->Ain_floor_lochblech = (float)((3 * 35 + 2 * 34) * 7 * M_PI
                                         * sqr(0.009 / 2));

    fprintf(stderr, "A_4holes=%8.5f\n", model->Ain_floor_4holes);
    fprintf(stderr, "A_4holesopen=%8.5f\n", model->Ain_floor_4holesopen);
    fprintf(stderr, "A_1hole=%8.5f\n", model->Ain_floor_1hole);
    fprintf(stderr, "A_lochblech=%8.5f\n", model->Ain_floor_lochblech);

    model->Ain_floor_total = model->Ain_floor_4holes * model->n_floor_4holes
                             + model->Ain_floor_4holesopen * model->n_floor_4holesopen
                             + model->Ain_floor_1hole * model->n_floor_1hole
                             + model->Ain_floor_lochblech * model->n_floor_lochblech;

    fprintf(stderr, "Ain_floor_total=%8.5f\n", model->Ain_floor_total);

    model->vin_4holes = (model->Ain_floor_4holes / model->Ain_floor_total)
                        * (model->Q_total) / (3600.0f * sqr(0.6f));
    model->vin_4holesopen = (model->Ain_floor_4holesopen
                             / model->Ain_floor_total) * (model->Q_total)
                            / (3600.0f * sqr(0.6f));
    model->vin_1hole = (model->Ain_floor_1hole / model->Ain_floor_total)
                       * (model->Q_total) / (3600.0f * sqr(0.6f));
    model->vin_lochblech = (model->Ain_floor_lochblech / model->Ain_floor_total)
                           * (model->Q_total) / (3600.0f * sqr(0.6f));

    fprintf(stderr, "vin_4holes=%8.5f\n", model->vin_4holes);
    fprintf(stderr, "vin_4holesopen=%8.5f\n", model->vin_4holesopen);
    fprintf(stderr, "vin_1hole=%8.5f\n", model->vin_1hole);
    fprintf(stderr, "vin_lochblech=%8.5f\n", model->vin_lochblech);

    fprintf(stderr, "Q_4holes=%8.5f\n",
            model->vin_4holes * model->Ain_floor_4holes
            * model->n_floor_4holes);
    fprintf(stderr, "Q_4holesopen=%8.5f\n",
            model->vin_4holesopen * model->Ain_floor_4holesopen
            * model->n_floor_4holesopen);
    fprintf(stderr, "Q_1hole=%8.5f\n",
            model->vin_1hole * model->Ain_floor_1hole * model->n_floor_1hole);
    fprintf(stderr, "Q_lochblech=%8.5f\n",
            model->vin_lochblech * model->Ain_floor_lochblech
            * model->n_floor_lochblech);

    model->Ain_floor_total = model->Ain_floor_4holes * model->n_floor_4holes

                             + model->Ain_floor_4holesopen * model->n_floor_4holesopen
                             + model->Ain_floor_1hole * model->n_floor_1hole
                             + model->Ain_floor_lochblech * model->n_floor_lochblech;

    for (i = 0; i < xpos_4holes.size(); i++) // the floor squares
    {
        for (j = 0; j < spacing; j++) // x-direction
        {
            for (k = 0; k < spacing; k++) // y-direction
            {
                grid->bcinlet_4holes_pol.push_back(
                    (int)grid->bcinlet_4holes.size());

                grid->bcinlet_4holes.push_back(
                    (ypos_4holes[i] + k) * grid->npoi_x
                    + (xpos_4holes[i] + j));
                grid->bcinlet_4holes.push_back(
                    (ypos_4holes[i] + k) * grid->npoi_x
                    + (xpos_4holes[i] + j) + 1);
                grid->bcinlet_4holes.push_back(
                    (ypos_4holes[i] + k + 1) * grid->npoi_x
                    + (xpos_4holes[i] + j) + 1);
                grid->bcinlet_4holes.push_back(
                    (ypos_4holes[i] + k + 1) * grid->npoi_x
                    + (xpos_4holes[i] + j));

                grid->bcinlet_4holes_vol.push_back(
                    (ypos_4holes[i] + k) * grid->nelem_x
                    + (xpos_4holes[i] + j));
                grid->bcinlet_4holes_outer.push_back(-1);

                grid->bcin_type.push_back(150);
                grid->bcin_type2.push_back(1.);
            }
        }
    }

    for (i = 0; i < xpos_4holesopen.size(); i++) // the floor squares
    {
        for (j = 0; j < spacing; j++) // x-direction
        {
            for (k = 0; k < spacing; k++) // y-direction
            {
                grid->bcinlet_4holesopen_pol.push_back(
                    (int)grid->bcinlet_4holesopen.size());

                grid->bcinlet_4holesopen.push_back(
                    (ypos_4holesopen[i] + k) * grid->npoi_x
                    + (xpos_4holesopen[i] + j));
                grid->bcinlet_4holesopen.push_back(
                    (ypos_4holesopen[i] + k) * grid->npoi_x
                    + (xpos_4holesopen[i] + j) + 1);
                grid->bcinlet_4holesopen.push_back(
                    (ypos_4holesopen[i] + k + 1) * grid->npoi_x
                    + (xpos_4holesopen[i] + j) + 1);
                grid->bcinlet_4holesopen.push_back(
                    (ypos_4holesopen[i] + k + 1) * grid->npoi_x
                    + (xpos_4holesopen[i] + j));

                grid->bcinlet_4holesopen_vol.push_back(
                    (ypos_4holesopen[i] + k) * grid->nelem_x
                    + (xpos_4holesopen[i] + j));
                grid->bcinlet_4holesopen_outer.push_back(-1);

                grid->bcin_type.push_back(154);
                grid->bcin_type2.push_back(2.);
            }
        }
    }

    for (i = 0; i < xpos_1hole.size(); i++) // the floor squares
    {
        for (j = 0; j < spacing; j++) // x-direction
        {
            for (k = 0; k < spacing; k++) // y-direction
            {
                grid->bcinlet_1hole_pol.push_back(
                    (int)grid->bcinlet_1hole.size());

                grid->bcinlet_1hole.push_back(
                    (ypos_1hole[i] + k) * grid->npoi_x
                    + (xpos_1hole[i] + j));
                grid->bcinlet_1hole.push_back(
                    (ypos_1hole[i] + k) * grid->npoi_x + (xpos_1hole[i] + j)
                    + 1);
                grid->bcinlet_1hole.push_back(
                    (ypos_1hole[i] + k + 1) * grid->npoi_x
                    + (xpos_1hole[i] + j) + 1);
                grid->bcinlet_1hole.push_back(
                    (ypos_1hole[i] + k + 1) * grid->npoi_x
                    + (xpos_1hole[i] + j));

                grid->bcinlet_1hole_vol.push_back(
                    (ypos_1hole[i] + k) * grid->nelem_x
                    + (xpos_1hole[i] + j));
                grid->bcinlet_1hole_outer.push_back(-1);

                grid->bcin_type.push_back(151);
                grid->bcin_type2.push_back(4.);
            }
        }
    }

    for (i = 0; i < xpos_lochblech.size(); i++) // the floor squares
    {
        for (j = 0; j < spacing; j++) // x-direction
        {
            for (k = 0; k < spacing; k++) // y-direction
            {
                grid->bcinlet_lochblech_pol.push_back(
                    (int)grid->bcinlet_lochblech.size());

                grid->bcinlet_lochblech.push_back(
                    (ypos_lochblech[i] + k) * grid->npoi_x
                    + (xpos_lochblech[i] + j));
                grid->bcinlet_lochblech.push_back(
                    (ypos_lochblech[i] + k) * grid->npoi_x
                    + (xpos_lochblech[i] + j) + 1);
                grid->bcinlet_lochblech.push_back(
                    (ypos_lochblech[i] + k + 1) * grid->npoi_x
                    + (xpos_lochblech[i] + j) + 1);
                grid->bcinlet_lochblech.push_back(
                    (ypos_lochblech[i] + k + 1) * grid->npoi_x
                    + (xpos_lochblech[i] + j));

                grid->bcinlet_lochblech_vol.push_back(
                    (ypos_lochblech[i] + k) * grid->nelem_x
                    + (xpos_lochblech[i] + j));
                grid->bcinlet_lochblech_outer.push_back(-1);

                grid->bcin_type.push_back(153);
                grid->bcin_type2.push_back(3.);
            }
        }
    }

    return 0;
}

int GenerateCeilingBCs(struct rechgrid *grid, struct rech_model *model)
{
    // here we want to generate the ceiling outlet BCs
    fprintf(stderr, "GenerateCeilingBCs!\n");

    float x, y;
#if NONCOMPLIENTPARSING
    char* token;
    int kx, jy;
#endif
    int  jhi;
    int i, j, k;

    int jlo; // y-position in structured grid
    int klo, khi; // x-position

    vector<float> dx;
    vector<float> dy;

    vector<int> angle;

    int spacing = (int)model->spacing;

    //read from PLMXML-File
    plmxml_file = model->PLMXMLFile;
    document = myParser.parse(plmxml_file, document);

    float room_xmax, room_ymax;
    room_xmax = grid->spacing_x * grid->nelem_x;
    room_ymax = grid->spacing_y * grid->nelem_y;

    //  if ((stream = fopen(model->BCFile, "r")) == NULL)
    //    {
    //      fprintf(stderr, "Cannot open '%s'! Does file exist?\n", model->BCFile);
    //      return -1;
    //    }
    vector<float> xpos_ceiling;
    vector<float> ypos_ceiling;

    int npoi_x = grid->npoi_x;
    int npoi_y = grid->npoi_y;

    float dymin;
    float dymax;

    float dxmin;
    float dxmax;

    float xmin, xmax;
    float ymin, ymax;

    float tol;

    grid->ceilingbcs = new int[grid->nelem_x * grid->nelem_y];
    memset(grid->ceilingbcs, 0, sizeof(int) * grid->nelem_x * grid->nelem_y);
#if NONCOMPLIENTPARSING
    // generate the BCs

    //  fgets(buf, 2000, stream);
    //  while (strncmp(buf, "CEILING", 7))   // Oulets in ceiling
    //    {
    //      fgets(buf, 2000, stream);
    //    }

    //  fgets(buf, 2000, stream);
    //  fgets(buf, 2000, stream);

    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_ceilx), 5))
        {
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            if ((x >= -0.00001) && ((x + dx) <= room_xmax))
                            {
                                xpos_ceiling.push_back(x);
                            }
                        }
                    }
                }
            }
        }
    }

    listInst = document->getElementsByTagName(TAG_ProductInstance);
    for (int ii = 0; ii < listInst->getLength(); ii++)
    {
        DOMElement *element = dynamic_cast<DOMElement *>(listInst->item(ii));
        if (!strncmp(XMLString::transcode(element->getAttribute(TAG_id)),
                     XMLString::transcode(TAG_ceily), 5))
        {
            if (element->hasChildNodes())
            {
                DOMNodeList *children = element->getChildNodes();
                for (int iiii = 0; iiii < children->getLength(); iiii++)
                {
                    char *posString = XMLString::transcode(
                        children->item(iiii)->getNodeName());
                    if (!strncmp(posString, XMLString::transcode(TAG_transform),
                                 9))
                    {
                        DOMElement *eChild = dynamic_cast<DOMElement *>(children->item(iiii));
                        char *transString = XMLString::transcode(
                            eChild->getTextContent());
                        double mat[16];
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            y = mat[13];
                            if ((y >= -0.00001) && ((y + dy) <= room_ymax))
                            {
                                ypos_ceiling.push_back(y);
                            }
                        }
                    }
                }
            }
        }
    }

    //	int npoi_x = grid->npoi_x;
    //	int npoi_y = grid->npoi_y;
    //
    //	float dymin;
    //	float dymax;
    //
    //	float dxmin;
    //	float dxmax;
    //
    //	float xmin, xmax;
    //	float ymin, ymax;
    //
    //	float tol;

    i = grid->nelem_z; // we are always in z-most grid layer (ceiling)

    grid->ceilingbcs = new int[grid->nelem_x * grid->nelem_y];
    memset(grid->ceilingbcs, 0, sizeof(int) * grid->nelem_x * grid->nelem_y);

    for (jy = 0; jy < ypos_ceiling.size(); jy++)
    {
        for (kx = 0; kx < xpos_ceiling.size(); kx++)
        {
            jlo = (int)(ceil((ypos_ceiling[jy]) / grid->spacing_y));
            jhi = (int)(floor((ypos_ceiling[jy] + dy) / grid->spacing_y));

            ymin = ypos_ceiling[jy];
            ymax = ypos_ceiling[jy] + dy;

            xmin = xpos_ceiling[kx];
            xmax = xpos_ceiling[kx] + dx;

            klo = (int)(ceil((xpos_ceiling[kx]) / grid->spacing_x));
            khi = (int)(floor((xpos_ceiling[kx] + dx) / grid->spacing_x));

            if (klo == khi)
            {
                if ((klo * grid->spacing_x - xpos_ceiling[kx])
                    > (xpos_ceiling[kx] + dx - khi * grid->spacing_x))
                {
                    klo--;
                }
                else
                {
                    khi++;
                }
            }

            // adapt coords of ceiling margins so that grid fits to real margins
            // real margins of ceiling: xmin-xmax, ymin-ymax
            // margins (completely inside cube): jlo-jhi, klo-khi
            // grid->spacing_x, grid->spacing_y

            dymin = jlo * grid->spacing_y - ymin;
            dymax = ymax - (jhi * grid->spacing_y);

            dxmin = klo * grid->spacing_x - xmin;
            dxmax = xmax - (khi * grid->spacing_x);

            if (dymin < (-0.000001))
                fprintf(stderr, "error in GenerateCeilingBCs! dymin<0\n");
            if (dymax < (-0.000001))
                fprintf(stderr,
                        "error in GenerateCeilingBCs! dymax<0. jy=%d, kx=%d\n",
                        jy, kx);

            if (dxmin < (-0.000001))
                fprintf(stderr, "error in GenerateCeilingBCs! dxmin<0\n");
            if (dxmax < (-0.000001))
                fprintf(stderr, "error in GenerateCeilingBCs! dxmax<0\n");

            tol = 0.4f;

            // two possibilities:
            // 1. make cells outside larger
            // 2. make cells outside smaller
            // tol < 0.5: cells outside get larger
            // tol > 0.5: cells outside get smaller: quad with angles > 90 degrees possible!

            if (dymin > tol * grid->spacing_y)
            {
                // move jlo-1 backwards
                jlo--;
            }

            if (dymax > tol * grid->spacing_y)
            {
                // move jhi+1 frontwards
                jhi++;
            }

            if (dxmin > tol * grid->spacing_x)
            {
                // move klo-1 right
                klo--;
            }
            if (dxmax > tol * grid->spacing_x)
            {
                // move khi+1 left
                khi++;
            }

            // now move it ...

            if (dymin <= tol * grid->spacing_y)
            {
                // move jlo frontwards
                j = jlo;
                for (k = klo; k <= khi; k++)
                {
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= dymin;
                }
            }
            else
            {
                // move jlo dy backwards
                j = jlo;
                for (k = klo; k <= khi; k++)
                {
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += grid->spacing_y - dymin;
                }
            }

            if (dxmin <= tol * grid->spacing_x)
            {
                // move klo dx left
                k = klo;
                for (j = jlo; j <= jhi; j++)
                {
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= dxmin;
                }
            }
            else
            {
                // move klo dx right
                k = klo;
                for (j = jlo; j <= jhi; j++)
                {
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += grid->spacing_x - dxmin;
                }
            }

            if (dymax <= tol * grid->spacing_y)
            {
                // move jhi backwards
                j = jhi;
                for (k = klo; k <= khi; k++)
                {
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += dymax;
                }
            }
            else
            {
                // move jhi dy frontwards
                j = jhi;
                for (k = klo; k <= khi; k++)
                {
                    grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= grid->spacing_y - dymax;
                }
            }

            if (dxmax <= tol * grid->spacing_x)
            {
                // move khi dx right
                k = khi;
                for (j = jlo; j <= jhi; j++)
                {
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += dxmax;
                }
            }
            else
            {
                // move khi dx left
                k = khi;
                for (j = jlo; j <= jhi; j++)
                {
                    grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= grid->spacing_x - dxmax;
                }
            }

            // generate BCs!!!!!!!
            // we have:
            // x-direction klo-khi
            // y-direction jlo-jhi
            // std::vector<int> bcoutlet_air;
            // std::vector<int> bcoutlet_air_pol;
            // std::vector<int> bcoutlet_air_vol;
            // std::vector<int> bcoutlet_air_outer;
            // std::vector<int> bcoutlet_air_climanr;  // we have eight separate airconditionings that can be treated independently

            int point_offset = i * (grid->npoi_x * grid->npoi_y);
            int elem_offset = (i - 1) * (grid->nelem_x * grid->nelem_y);

            for (k = klo; k < khi; k++) // x
            {
                for (j = jlo; j < jhi; j++) // y
                {
                    grid->bcoutlet_air.push_back(
                        point_offset + j * grid->npoi_x + k);
                    grid->bcoutlet_air.push_back(
                        point_offset + j * grid->npoi_x + k + 1);
                    grid->bcoutlet_air.push_back(
                        point_offset + (j + 1) * grid->npoi_x + k + 1);
                    grid->bcoutlet_air.push_back(
                        point_offset + (j + 1) * grid->npoi_x + k);
                    grid->bcoutlet_air_pol.push_back(
                        4 * (int)grid->bcoutlet_air_pol.size());
                    grid->bcoutlet_air_vol.push_back(
                        elem_offset + j * grid->nelem_x + k);
                    grid->bcoutlet_air_outer.push_back(-1);
                    grid->ceilingbcs[j * grid->nelem_x + k] = (kx + 2) / 2;

                    grid->bcout_type.push_back(300 + (kx) / 2);
                }
            }
        }
    }
#else
    listUserData = document->getElementsByTagName(TAG_UserData);
    listInst = document->getElementsByTagName(TAG_transform);
    listBounds = document->getElementsByTagName(TAG_Bound);
    for (int it = 0; it < listUserData->getLength(); it++)
    {
        DOMElement *curEle = dynamic_cast<DOMElement *>(listUserData->item(it));
        DOMElement *curPar = dynamic_cast<DOMElement *>(curEle->getParentNode());
        double mat[16];
        double b[6];
        string parName = XMLString::transcode(curPar->getAttribute(TAG_name));
        if (curEle)
        {
            string curID;
            for (DOMNode *child = curEle->getFirstChild(); child != NULL;
                 child = child->getNextSibling())
            {
                DOMElement *curChild = dynamic_cast<DOMElement *>(child);

                if (curChild)
                {
                    if ((XMLString::compareString(
                             curChild->getAttribute(TAG_title), TAG_DEBBLevel)
                         == 0)
                        && (XMLString::compareString(
                                curChild->getAttribute(TAG_value),
                                TAG_Outlet) == 0)
                        && (strstr(parName.c_str(), "FlowPump") != NULL))
                    {
                        curID = XMLString::transcode(
                            curEle->getAttribute(TAG_id));
                        cerr
                            << XMLString::transcode(
                                   curChild->getAttribute(TAG_title))
                            << " "
                            << XMLString::transcode(
                                   curChild->getAttribute(TAG_value))
                            << endl;
                        break;
                    }
                }
            }
            if (curID.empty())
                continue;
            cerr << curID << " is an Outlet!" << endl;
            for (int ii = 0; ii < listInst->getLength(); ii++)
            {

                DOMElement *curInst = dynamic_cast<DOMElement *>(listInst->item(
                    ii));
                if (curInst)
                {
                    string idInst = XMLString::transcode(
                        curInst->getAttribute(TAG_id));
                    idInst.erase(0, 2);
                    if (strstr(curID.c_str(), idInst.c_str()) != NULL)
                    {
                        cerr << curID << " contains " << idInst << endl;
                        curID = idInst;
                        char *transString = XMLString::transcode(
                            curInst->getTextContent());
                        if (sscanf(transString,
                                   "%lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                   &mat[0], &mat[1], &mat[2], &mat[3], &mat[4],
                                   &mat[5], &mat[6], &mat[7], &mat[8], &mat[9],
                                   &mat[10], &mat[11], &mat[12], &mat[13],
                                   &mat[14], &mat[15]) == 16)
                        {
                            x = mat[12];
                            y = mat[13];

                            if (mat[0] > 0) // angle=0
                                angle.push_back(0);
                            if (mat[1] < 0) // angle=90
                                angle.push_back(90);
                            if (mat[0] < 0) // angle=180
                                angle.push_back(180);
                            if (mat[1] > 0) // angle=270
                                angle.push_back(270);
                        }
                        break;
                    }
                }
            }
            DOMNode *curSib = curEle->getNextSibling();
            while (curSib != NULL)
            {
                if (curSib->getNodeType() == DOMNode::ELEMENT_NODE)
                {
                    DOMElement *bound = dynamic_cast<DOMElement *>(curSib);
                    if (XMLString::compareString(bound->getNodeName(),
                                                 TAG_Bound) == 0)
                    {
                        if (bound)
                        {
                            string boundID = XMLString::transcode(
                                bound->getAttribute(TAG_id));
                            if (strstr(boundID.c_str(), curID.c_str()) != NULL)
                            {
                                cerr << boundID << " also matches!" << endl;
                                char *boundString = XMLString::transcode(
                                    bound->getAttribute(TAG_values));
                                if (sscanf(boundString,
                                           "%lg %lg %lg %lg %lg %lg ", &b[0],
                                           &b[1], &b[2], &b[3], &b[4], &b[5]))
                                    printf(
                                        "Outlet found at: (%lg,%lg,%lg) with size: (%lg,%lg,%lg)\n",
                                        mat[12], mat[13], mat[14], b[3],
                                        b[4], b[5]);

                                break;
                            }
                        }
                    }
                }
                curSib = curSib->getNextSibling();
            }
            if ((x >= -0.00001) && ((x + 2 * b[3]) <= room_xmax))
            {
                xpos_ceiling.push_back(x);
                dx.push_back(2 * b[3]);
            }
            if ((y >= -0.00001) && ((y + 2 * b[4]) <= room_ymax))
            {
                ypos_ceiling.push_back(y);
                dy.push_back(2 * b[4]);
            }
        }
    }

    if ((xpos_ceiling.size() != ypos_ceiling.size())
        || (dx.size() != dy.size()))
    {
        printf("Error in Ceiling BCS, size of x and y values not equal!");
        return 0;
    }

    for (int it = 0; it < xpos_ceiling.size(); it++)
    {
        switch (angle[it])
        {
        case 0:
            break;
        case 90:
            ypos_ceiling[it] -= dx[it];
            std::swap(dx[it], dy[it]);
            break;
        case 180:
            xpos_ceiling[it] -= dx[it];
            ypos_ceiling[it] -= dy[it];
            break;
        case 270:
            xpos_ceiling[it] -= dy[it];
            std::swap(dx[it], dy[it]);
            break;
        default:
            printf("Faulty angle value!");
        }
    }

    i = grid->nelem_z;
    // ceiling BCS NOT! treated as combinations of x and y values
    for (int it = 0; it < xpos_ceiling.size(); it++)
    {

        // determine elements at the boundaries of the outlet
        jlo = (int)(ceil((ypos_ceiling[it]) / grid->spacing_y));
        jhi = (int)(floor((ypos_ceiling[it] + dy[it]) / grid->spacing_y));

        klo = (int)(ceil((xpos_ceiling[it]) / grid->spacing_x));
        khi = (int)(floor((xpos_ceiling[it] + dx[it]) / grid->spacing_x));

        // have coordinate bounds in a more handy way
        ymin = ypos_ceiling[it];
        ymax = ymin + dy[it];

        xmin = xpos_ceiling[it];
        xmax = xmin + dx[it];

        // if elemental bounds are equal, move the bound towards the farthest exact bound
        if (klo == khi)
        {
            if ((klo * grid->spacing_x - xmin) > (xmax - khi * grid->spacing_x))
                klo--;
            else
                khi++;
        }

        if (jlo == jhi)
        {
            if ((jlo * grid->spacing_y - ymin) > (ymax - jhi * grid->spacing_y))
                jlo--;
            else
                jhi++;
        }
        //offset between grid bounds and real bounds
        dymin = jlo * grid->spacing_y - ymin;
        dymax = ymax - jhi * grid->spacing_y;

        dxmin = klo * grid->spacing_x - xmin;
        dxmax = xmax - khi * grid->spacing_x;

        if (dymin < (-0.000001))
            fprintf(stderr, "error in GenerateCeilingBCs! dymin<0\n");
        if (dymax < (-0.000001))
            fprintf(stderr, "error in GenerateCeilingBCs! dymax<0\n");
        if (dxmin < (-0.000001))
            fprintf(stderr, "error in GenerateCeilingBCs! dxmin<0\n");
        if (dxmax < (-0.000001))
            fprintf(stderr, "error in GenerateCeilingBCs! dxmax<0\n");

        tol = 0.4f;

        if (dymin > tol * grid->spacing_y)
        {
            // move jlo-1 backwards
            jlo--;
        }

        if (dymax > tol * grid->spacing_y)
        {
            // move jhi+1 frontwards
            jhi++;
        }

        if (dxmin > tol * grid->spacing_x)
        {
            // move klo-1 right
            klo--;
        }
        if (dxmax > tol * grid->spacing_x)
        {
            // move khi+1 left
            khi++;
        }

        if (dymin <= tol * grid->spacing_y)
        {
            // move jlo frontwards
            j = jlo;
            for (k = klo; k <= khi; k++)
                grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= dymin;
        }
        else
        {
            // move jlo dy backwards
            j = jlo;
            for (k = klo; k <= khi; k++)
                grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += grid->spacing_y - dymin;
        }

        if (dxmin <= tol * grid->spacing_x)
        {
            // move klo dx left
            k = klo;
            for (j = jlo; j <= jhi; j++)
                grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= dxmin;
        }
        else
        {
            // move klo dx right
            k = klo;
            for (j = jlo; j <= jhi; j++)
                grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += grid->spacing_x - dxmin;
        }

        if (dymax <= tol * grid->spacing_y)
        {
            // move jhi backwards
            j = jhi;
            for (k = klo; k <= khi; k++)
                grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += dymax;
        }
        else
        {
            // move jhi dy frontwards
            j = jhi;
            for (k = klo; k <= khi; k++)
                grid->p->y[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= grid->spacing_y - dymax;
        }

        if (dxmax <= tol * grid->spacing_x)
        {
            // move khi dx right
            k = khi;
            for (j = jlo; j <= jhi; j++)
                grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] += dxmax;
        }
        else
        {
            // move khi dx left
            k = khi;
            for (j = jlo; j <= jhi; j++)
                grid->p->x[i * (npoi_x * npoi_y) + j * (npoi_x) + k] -= grid->spacing_x - dxmax;
        }

        int point_offset = i * (grid->npoi_x * grid->npoi_y);
        int elem_offset = (i - 1) * (grid->nelem_x * grid->nelem_y);

        for (k = klo; k < khi; k++) // x
        {
            for (j = jlo; j < jhi; j++) // y
            {
                grid->bcoutlet_air.push_back(
                    point_offset + j * grid->npoi_x + k);
                grid->bcoutlet_air.push_back(
                    point_offset + j * grid->npoi_x + k + 1);
                grid->bcoutlet_air.push_back(
                    point_offset + (j + 1) * grid->npoi_x + k + 1);
                grid->bcoutlet_air.push_back(
                    point_offset + (j + 1) * grid->npoi_x + k);
                grid->bcoutlet_air_pol.push_back(
                    4 * (int)grid->bcoutlet_air_pol.size());
                grid->bcoutlet_air_vol.push_back(
                    elem_offset + j * grid->nelem_x + k);
                grid->bcoutlet_air_outer.push_back(-1);
                grid->ceilingbcs[j * grid->nelem_x + k] = it + 1;

                grid->bcout_type.push_back(300 + it);
            }
        }
    }
//	for (int it = 0; it < (grid->nelem_x * grid->nelem_y); it++) {
//		cerr << grid->ceilingbcs[it];
//		if ((it + 1) % grid->nelem_x == 0) {
//			cerr << endl;
//		}
//	}
#endif
    return 0;
}

int GenerateBCs(struct rechgrid *grid, struct rech_model *model, int number)
{
    /*
	 float xmin = -0.5*model->size[0];
	 float xmax =  0.5*model->size[0];
	 float ymin = -0.5*model->size[1];
	 float ymax =  0.5*model->size[1];
	 float zmin = -0.5*model->size[2];
	 float zmax =  0.5*model->size[2];
	 */
    int npoi_x = grid->npoi_x;
    int npoi_y = grid->npoi_y;
    int npoi_z = grid->npoi_z;
    int nelem_x = grid->nelem_x;
    int nelem_y = grid->nelem_y;
    //	int nelem_z = grid->nelem_z;

    cout << "generateBCS number=" << number << endl;
    cout << "---" << endl;

    int bcminusx = 0;
    int bcplusx = 0;
    int bcminusy = 0;
    int bcplusy = 0;
    int bcminusz = 0;
    int bcplusz = 0;

    int ilo, ihi, jlo, jhi, klo, khi;
    int i, j, k;

    if (number != -1)
    {
        // this is a cube to subtract
        ilo = model->cubes[number]->ilo;
        ihi = model->cubes[number]->ihi;
        jlo = model->cubes[number]->jlo;
        jhi = model->cubes[number]->jhi;
        klo = model->cubes[number]->klo;
        khi = model->cubes[number]->khi;
        bcminusx = model->cubes[number]->bc_type_minusx;
        bcplusx = model->cubes[number]->bc_type_plusx;
        bcminusy = model->cubes[number]->bc_type_minusy;
        bcplusy = model->cubes[number]->bc_type_plusy;
        bcminusz = model->cubes[number]->bc_type_minusz;
        bcplusz = model->cubes[number]->bc_type_plusz;
    }
    else
    {
        //ilo,ihi,...: elements!!!
        ilo = model->ilo;
        ihi = model->ihi;
        jlo = model->jlo;
        jhi = model->jhi;
        klo = model->klo;
        khi = model->khi;
        bcminusx = model->bc_type_minusx;
        bcplusx = model->bc_type_plusx;
        bcminusy = model->bc_type_minusy;
        bcplusy = model->bc_type_plusy;
        bcminusz = model->bc_type_minusz;
        bcplusz = model->bc_type_plusz;
    }
    //	fprintf(stderr, "ilo: %d,ihi: %d, jlo: %d, jhi: %d, klo: %d, khi: %d\n", ilo,
    //			ihi, jlo, jhi, klo, khi);
    //	fprintf(stderr,
    //			"bcminusx: %d, bcplusx: %d, bcminusy: %d, bcplusy %d, bcminusz: %d, bcplusz: %d\n ---------- \n",
    //			bcminusx, bcplusx, bcminusy, bcplusy, bcminusz, bcplusz);
    int elem[6];

    // WALL WALL WALL
    // WALL WALL WALL
    // WALL WALL WALL
    // WALL WALL WALL

    // wall -x side
    if (bcminusx == WALL)
    {
        k = klo;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }
                Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                Add2Ilist(grid->bcwall, elem[0]);
                Add2Ilist(grid->bcwall, elem[1]);
                Add2Ilist(grid->bcwall, elem[2]);
                Add2Ilist(grid->bcwall, elem[3]);
                //fprintf(stderr,"klo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcwallvol, elem[4]);
                Add2Ilist(grid->bcwallvol_outer, elem[5]);
                if (elem[5] > grid->nelem)
                    fprintf(stderr, "error 1!\t");
            }
        }
    }
    /*
	 // inlet node bcs
	 if (number==-1)
	 {
	 grid->bcin_nodes=AllocIlistStruct(25);
	 grid->bcin_velos=AllocFlistStruct(125);

	 int bcnode;

	 // Node-bc inlet (outer geometry)
	 k=0;

	 float l = 4.;	// characteristic length
	 float _k   = 3.75 * 0.001  * pow(model->bcin_velo,2);
	 float _eps = 9.40 * 0.0001 * pow(fabs(model->bcin_velo/l),3);

	 for (i=ilo;i<ihi;i++)
	 {
	 for (j=jlo;j<jhi;j++)
	 {
	 bcnode = i*(npoi_x*npoi_y)+j*(npoi_x)+k;
	 Add2Ilist(grid->bcin_nodes, bcnode);
	 Add2Flist(grid->bcin_velos, model->bcin_velo);
	 Add2Flist(grid->bcin_velos, 0.0);
	 Add2Flist(grid->bcin_velos, 0.0);
	 Add2Flist(grid->bcin_velos, _k);
	 Add2Flist(grid->bcin_velos, _eps);
	 }
	 }
	 }
	 */

    // wall +x side
    if (bcplusx == WALL)
    {
        k = khi;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                Add2Ilist(grid->bcwall, elem[0]);
                Add2Ilist(grid->bcwall, elem[1]);
                Add2Ilist(grid->bcwall, elem[2]);
                Add2Ilist(grid->bcwall, elem[3]);
                //fprintf(stderr,"khi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcwallvol, elem[4]);
                Add2Ilist(grid->bcwallvol_outer, elem[5]);
                if (elem[5] > grid->nelem)
                    fprintf(stderr, "error 2!\t");
            }
        }
    }

    // wall -y side
    if (bcminusy == WALL)
    {
        j = jlo;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }
                Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                Add2Ilist(grid->bcwall, elem[0]);
                Add2Ilist(grid->bcwall, elem[1]);
                Add2Ilist(grid->bcwall, elem[2]);
                Add2Ilist(grid->bcwall, elem[3]);
                //fprintf(stderr,"jlo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcwallvol, elem[4]);
                Add2Ilist(grid->bcwallvol_outer, elem[5]);
                if (elem[5] > grid->nelem)
                    fprintf(stderr, "error 3!\t");
            }
        }
    }

    // wall +y side
    if (bcplusy == WALL)
    {
        j = jhi;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                Add2Ilist(grid->bcwall, elem[0]);
                Add2Ilist(grid->bcwall, elem[1]);
                Add2Ilist(grid->bcwall, elem[2]);
                Add2Ilist(grid->bcwall, elem[3]);
                //fprintf(stderr,"jhi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcwallvol, elem[4]);
                Add2Ilist(grid->bcwallvol_outer, elem[5]);
                if (elem[5] > grid->nelem)
                    fprintf(stderr, "error 4!\t");
            }
        }
    }

    // wall -z side
    if (bcminusz == WALL)
    {
        i = ilo;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[5] = -1;
                    else
                        elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[4] = -1;
                    else
                        elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }
                // only add if this is not an inlet face!!!!!
                if (!((i == 0) && (grid->floorbcs[j * grid->nelem_x + k] != 0)))
                {
                    Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                    Add2Ilist(grid->bcwall, elem[0]);
                    Add2Ilist(grid->bcwall, elem[1]);
                    Add2Ilist(grid->bcwall, elem[2]);
                    Add2Ilist(grid->bcwall, elem[3]);
                    //fprintf(stderr,"ilo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                    Add2Ilist(grid->bcwallvol, elem[4]);
                    Add2Ilist(grid->bcwallvol_outer, elem[5]);
                    if (elem[5] > grid->nelem)
                        fprintf(stderr, "error 5!\t");
                }
            }
        }
    }

    // wall +z side
    if (bcplusz == WALL)
    {
        i = ihi;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;
                if (number == -1)
                {
                    elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                if (!((i == grid->nelem_z)
                      && (grid->ceilingbcs[j * grid->nelem_x + k] != 0)))
                {
                    Add2Ilist(grid->bcwallpol, grid->bcwall->num);
                    Add2Ilist(grid->bcwall, elem[0]);
                    Add2Ilist(grid->bcwall, elem[1]);
                    Add2Ilist(grid->bcwall, elem[2]);
                    Add2Ilist(grid->bcwall, elem[3]);
                    //fprintf(stderr,"ihi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                    Add2Ilist(grid->bcwallvol, elem[4]);
                    Add2Ilist(grid->bcwallvol_outer, elem[5]);
                    if (elem[5] > grid->nelem)
                        fprintf(stderr, "error 6!\t");
                }
            }
        }
    }

    // INLET INLET
    // INLET INLET
    // INLET INLET
    // INLET INLET

    //int *inlet_nodes = (int*)calloc(grid->npoi, sizeof(int));

    // inlet -x side
    if (bcminusx == INLET + number) // das +number koennte man inzwischen auch weglassen, oder?
    {
        k = klo;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {

                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                //fprintf(stderr,"klo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }
    /*
	 // inlet node bcs
	 if (number==-1)
	 {
	 grid->bcin_nodes=AllocIlistStruct(25);
	 grid->bcin_velos=AllocFlistStruct(125);

	 int bcnode;

	 // Node-bc inlet (outer geometry)
	 k=0;

	 float l = 4.;	// characteristic length
	 float _k   = 3.75 * 0.001  * pow(model->bcin_velo,2);
	 float _eps = 9.40 * 0.0001 * pow(fabs(model->bcin_velo/l),3);

	 for (i=0; i<grid->npoi;i++)
	 {
	 if (inlet_nodes[i]==1)
	 {
	 bcnode = i;
	 Add2Ilist(grid->bcin_nodes, bcnode);
	 Add2Flist(grid->bcin_velos, model->bcin_velo);
	 Add2Flist(grid->bcin_velos, 0.0);
	 Add2Flist(grid->bcin_velos, 0.0);
	 Add2Flist(grid->bcin_velos, _k);
	 Add2Flist(grid->bcin_velos, _eps);
	 }
	 }
	 }
	 */

    // inlet +x side
    if (bcplusx == INLET + number)
    {
        k = khi;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }

    // inlet -y side
    if (bcminusy == INLET + number)
    {
        j = jlo;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                //fprintf(stderr,"jlo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }

    // inlet +y side
    if (bcplusy == INLET + number)
    {
        j = jhi;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                //fprintf(stderr,"jhi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }

    // inlet -z side
    if (bcminusz == INLET + number)
    {
        i = ilo;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;

                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[5] = -1;
                    else
                        elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[4] = -1;
                    else
                        elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                //fprintf(stderr,"ilo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }

    // inlet +z side
    if (bcplusz == INLET + number)
    {
        i = ihi;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;

                if (number == -1)
                {
                    elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }

                Add2Ilist(grid->bcinpol, grid->bcin->num);
                Add2Ilist(grid->bcin, elem[0]);
                Add2Ilist(grid->bcin, elem[1]);
                Add2Ilist(grid->bcin, elem[2]);
                Add2Ilist(grid->bcin, elem[3]);
                //fprintf(stderr,"ihi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                grid->bcinvol.push_back(elem[4]);
                Add2Ilist(grid->bcinvol_outer, elem[5]);
                grid->bcin_type.push_back(INLET + number);
                grid->bcin_type2.push_back(0.);
            }
        }
    }

    // OUTLET
    // OUTLET
    // OUTLET
    // OUTLET

    // outlet -x side
    if (bcminusx == OUTLET + number)
    {
        k = klo;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (k == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x)
                                  + (k - 1);
                }
                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"klo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    // outlet +x side
    if (bcplusx == OUTLET + number)
    {
        k = khi;
        for (i = ilo; i < ihi; i++)
        {
            for (j = jlo; j < jhi; j++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + npoi_x;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k - 1;
                    if (k == npoi_x - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }

                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"khi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    // outlet -y side
    if (bcminusy == OUTLET + number)
    {
        j = jlo;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (j == 0)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x)
                                  + k;
                }
                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"jlo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    // outlet +y side
    if (bcplusy == OUTLET + number)
    {
        j = jhi;
        for (i = ilo; i < ihi; i++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + (npoi_x * npoi_y);
                elem[3] = elem[0] + (npoi_x * npoi_y);
                elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + (j - 1) * (nelem_x) + k;
                    if (j == npoi_y - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }

                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"jhi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    // outlet -z side
    if (bcminusz == OUTLET + number)
    {
        i = ilo;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;
                elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                if (number == -1)
                {
                    elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[5] = -1;
                    else
                        elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }
                else
                {
                    elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == 0)
                        elem[4] = -1;
                    else
                        elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x)
                                  + k;
                }

                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"ilo[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    // outlet +z side
    if (bcplusz == OUTLET + number)
    {
        i = ihi;
        for (j = jlo; j < jhi; j++)
        {
            for (k = klo; k < khi; k++)
            {
                elem[0] = i * (npoi_x * npoi_y) + j * (npoi_x) + k;
                elem[1] = elem[0] + 1;
                elem[2] = elem[1] + npoi_x;
                elem[3] = elem[0] + npoi_x;
                elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                if (number == -1)
                {
                    elem[4] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[5] = -1;
                    else
                        elem[5] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                else
                {
                    elem[5] = (i - 1) * (nelem_x * nelem_y) + j * (nelem_x) + k;
                    if (i == npoi_z - 1)
                        elem[4] = -1;
                    else
                        elem[4] = i * (nelem_x * nelem_y) + j * (nelem_x) + k;
                }
                Add2Ilist(grid->bcoutpol, grid->bcout->num);
                Add2Ilist(grid->bcout, elem[0]);
                Add2Ilist(grid->bcout, elem[1]);
                Add2Ilist(grid->bcout, elem[2]);
                Add2Ilist(grid->bcout, elem[3]);
                //fprintf(stderr,"ihi[%d], added vertices %d %d %d %d\n", number, elem[0], elem[1], elem[2], elem[3]);
                Add2Ilist(grid->bcoutvol, elem[4]);
                Add2Ilist(grid->bcoutvol_outer, elem[5]);
                grid->bcout_type.push_back(OUTLET + number);
            }
        }
    }

    if (number == -1) // do this just once!
    {

        // inlet floor squares
        // add everything to grid->bcin
        // you might want to add this to a different array than grid->bcin

        // ATTENTION
        // grid->bcin_type and grid->bcout_type has already been filled
        // this can only work when no inlet and outlet bcs have been added to grid->bcin abd grid->bcout prior to this point!

        for (i = 0; i < grid->bcinlet_4holes_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_4holes[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holes[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holes[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holes[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_4holes_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }
        for (i = 0; i < grid->bcinlet_4holesopen_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_4holesopen[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holesopen[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holesopen[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_4holesopen[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_4holesopen_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }
        for (i = 0; i < grid->bcinlet_1hole_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_1hole[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_1hole[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_1hole[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_1hole[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_1hole_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }
        for (i = 0; i < grid->bcinlet_open_sx9_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_open_sx9[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_sx9[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_sx9[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_sx9[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_open_sx9_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }
        for (i = 0; i < grid->bcinlet_lochblech_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_lochblech[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_lochblech[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_lochblech[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_lochblech[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_lochblech_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }
        for (i = 0; i < grid->bcinlet_open_nec_cluster_vol.size(); i++)
        {
            Add2Ilist(grid->bcinpol, grid->bcin->num);
            Add2Ilist(grid->bcin, grid->bcinlet_open_nec_cluster[4 * i + 0]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_nec_cluster[4 * i + 1]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_nec_cluster[4 * i + 2]);
            Add2Ilist(grid->bcin, grid->bcinlet_open_nec_cluster[4 * i + 3]);
            grid->bcinvol.push_back(grid->bcinlet_open_nec_cluster_vol[i]);
            Add2Ilist(grid->bcinvol_outer, -1);
        }

        // outlet (clima)

        for (i = 0; i < grid->bcoutlet_air_vol.size(); i++)
        {
            Add2Ilist(grid->bcoutpol, grid->bcout->num);
            Add2Ilist(grid->bcout, grid->bcoutlet_air[4 * i + 0]);
            Add2Ilist(grid->bcout, grid->bcoutlet_air[4 * i + 1]);
            Add2Ilist(grid->bcout, grid->bcoutlet_air[4 * i + 2]);
            Add2Ilist(grid->bcout, grid->bcoutlet_air[4 * i + 3]);
            Add2Ilist(grid->bcoutvol, grid->bcoutlet_air_vol[i]);
            Add2Ilist(grid->bcoutvol_outer, -1);
        }
    }

    return 0;
}

int CreateGeoRbFile(struct rechgrid *grid, const char *geofile,
                    const char *rbfile)
{
    int i;

    FILE *stream;
    char file[512];

    // GEOFILE
    strcpy(file, geofile);
    if ((stream = fopen(&file[0], "w")) == NULL)
    {
        printf("Can't open %s for writing!\n", file);
    }
    else
    {
        for (i = 0; i < 10; i++)
        {
            fprintf(stream, "C\n");
        }
        fprintf(stream, "%d %d %d %d %d %d %d %d\n", grid->p->nump,
                grid->e->nume, 0, 0, 0, 0, grid->p->nump, grid->e->nume);

        for (i = 0; i < grid->p->nump; i++)
        {
            fprintf(stream, "%6d  %8.5f  %8.5f  %8.5f\n", i + 1, grid->p->x[i],
                    grid->p->y[i], grid->p->z[i]);
        }

        for (i = 0; i < grid->e->nume; i++)
        {
            fprintf(stream, "%10d %6d %6d %6d %6d %6d %6d %6d %6d\n", i + 1,
                    grid->e->e[i][0] + 1, grid->e->e[i][1] + 1,
                    grid->e->e[i][2] + 1, grid->e->e[i][3] + 1,
                    grid->e->e[i][4] + 1, grid->e->e[i][5] + 1,
                    grid->e->e[i][6] + 1, grid->e->e[i][7] + 1);
        }

        fclose(stream);
    }

    // RBFILE
    strcpy(file, rbfile);
    if ((stream = fopen(&file[0], "w")) == NULL)
    {
        printf("Can't open %s for writing!\n", file);
    }
    else
    {
        for (i = 0; i < 10; i++)
        {
            fprintf(stream, "C\n");
        }
        int n_bila = (int)grid->bcinvol.size() + grid->bcoutvol->num;
        int n_nodal = (int)grid->bcin_nodes.size();

        fprintf(stream, "%d %d %d %d %d %d %d %d\n", n_nodal * 5,
                grid->bcwallpol->num, 0, 0, 0, 0, n_bila, 0);

        for (i = 0; i < grid->bcin_nodes.size(); i++)
        {
            fprintf(stream, "%6d 1 %8.5f\n", grid->bcin_nodes[i] + 1,
                    grid->bcin_velos[5 * i + 0]);
            fprintf(stream, "%6d 2 %8.5f\n", grid->bcin_nodes[i] + 1,
                    grid->bcin_velos[5 * i + 1]);
            fprintf(stream, "%6d 3 %8.5f\n", grid->bcin_nodes[i] + 1,
                    grid->bcin_velos[5 * i + 2]);
            fprintf(stream, "%6d 4 %8.5f\n", grid->bcin_nodes[i] + 1,
                    grid->bcin_velos[5 * i + 3]);
            fprintf(stream, "%6d 5 %8.5f\n", grid->bcin_nodes[i] + 1,
                    grid->bcin_velos[5 * i + 4]);
        }

        for (i = 0; i < grid->bcwallvol->num; i++)
        {
            fprintf(stream, "%6d %6d %6d %6d %d %d %d %6d\n",
                    grid->bcwall->list[4 * i + 0] + 1,
                    grid->bcwall->list[4 * i + 1] + 1,
                    grid->bcwall->list[4 * i + 2] + 1,
                    grid->bcwall->list[4 * i + 3] + 1, 0, 0, 0,
                    grid->bcwallvol->list[i] + 1);
        }

        for (i = 0; i < grid->bcinvol.size(); i++)
        {
            fprintf(stream, "%6d %6d %6d %6d %6d %6d\n",
                    grid->bcin->list[4 * i + 0] + 1,
                    grid->bcin->list[4 * i + 1] + 1,
                    grid->bcin->list[4 * i + 2] + 1,
                    grid->bcin->list[4 * i + 3] + 1, grid->bcinvol[i] + 1, 100);
        }

        for (i = 0; i < grid->bcoutvol->num; i++)
        {
            fprintf(stream, "%6d %6d %6d %6d %6d %6d\n",
                    grid->bcout->list[4 * i + 0] + 1,
                    grid->bcout->list[4 * i + 1] + 1,
                    grid->bcout->list[4 * i + 2] + 1,
                    grid->bcout->list[4 * i + 3] + 1,
                    grid->bcoutvol->list[i] + 1, 200);
        }

        fclose(stream);
    }

    return 0;
}
