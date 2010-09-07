/*

Copyright (C) University of Oxford, 2005-2010

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef TESTVERTEXBASEDCELLPOPULATION_HPP_
#define TESTVERTEXBASEDCELLPOPULATION_HPP_

#include <cxxtest/TestSuite.h>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "CellwiseData.hpp"
#include "VertexBasedCellPopulation.hpp"
#include "HoneycombMutableVertexMeshGenerator.hpp"
#include "FixedDurationGenerationBasedCellCycleModel.hpp"
#include "WntCellCycleModel.hpp"
#include "CellwiseData.hpp"
#include "AbstractCellBasedTestSuite.hpp"
#include "ArchiveOpener.hpp"
#include "WildTypeCellMutationState.hpp"
#include "ApcOneHitCellMutationState.hpp"
#include "ApcTwoHitCellMutationState.hpp"
#include "BetaCateninOneHitCellMutationState.hpp"
#include "CellLabel.hpp"

class TestVertexBasedCellPopulation : public AbstractCellBasedTestSuite
{
private:

    /**
     * Set up cells, one for each VertexElement.
     * Give each cell a birth time of -elem_index,
     * so its age is elem_index.
     */
    template<unsigned DIM>
    std::vector<CellPtr> SetUpCells(MutableVertexMesh<DIM,DIM>& rMesh)
    {
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_state(CellPropertyRegistry::Instance()->Get<WildTypeCellMutationState>());
        for (unsigned i=0; i<rMesh.GetNumElements(); i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_model));
            double birth_time = 0.0 - i;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }
        return cells;
    }

public:

    // Test construction, accessors and iterator
    void TestCreateSmallVertexBasedCellPopulation() throw (Exception)
    {
        // Create a simple 2D VertexMesh
        HoneycombMutableVertexMeshGenerator generator(5, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);

        // Create cell population
        unsigned num_cells = cells.size();
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Test we have the correct number of cells and elements
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), p_mesh->GetNumElements());
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), num_cells);

        unsigned counter = 0;

        // Test VertexBasedCellPopulation::Iterator
        for (VertexBasedCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            // Test operator* and that cells are in sync
            TS_ASSERT_EQUALS(cell_population.GetLocationIndexUsingCell(*cell_iter), counter);

            // Test operator-> and that cells are in sync
            TS_ASSERT_DELTA(cell_iter->GetAge(), (double)counter, 1e-12);

            TS_ASSERT_EQUALS(counter, p_mesh->GetElement(counter)->GetIndex());

            counter++;
        }

        // Test we have gone through all cells in the for loop
        TS_ASSERT_EQUALS(counter, cell_population.GetNumRealCells());

        // Test GetNumNodes() method
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), p_mesh->GetNumNodes());
    }


    void TestValidate() throw (Exception)
    {
    	{
    		// Create a simple vertex-based mesh

			HoneycombMutableVertexMeshGenerator generator(3, 3);
			MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

			// Set up cells, one for each element.
			// Give each a birth time of -element_index, so the age = element_index.
			std::vector<CellPtr> cells;
			std::vector<unsigned> cell_location_indices;
			boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
			for (unsigned i=0; i<p_mesh->GetNumElements()-1; i++)
			{
				FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
				p_model->SetCellProliferativeType(STEM);

				CellPtr p_cell(new Cell(p_state, p_model));

				double birth_time = 0.0 - i;
				p_cell->SetBirthTime(birth_time);

				cells.push_back(p_cell);
				cell_location_indices.push_back(i);
			}

			// This should throw an exception as the number of cells
			// does not equal the number of elements
			std::vector<CellPtr> cells_copy(cells);
			TS_ASSERT_THROWS_THIS(VertexBasedCellPopulation<2> cell_population(*p_mesh, cells_copy),
					"Element 8 does not appear to have a cell associated with it");

			FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
			p_model->SetCellProliferativeType(STEM);

			CellPtr p_cell(new Cell(p_state, p_model));

			double birth_time = 0.0 - p_mesh->GetNumElements()-1;
			p_cell->SetBirthTime(birth_time);

			cells.push_back(p_cell);
			cell_location_indices.push_back(p_mesh->GetNumElements()-1);

			// This should pass as the number of cells equals the number of elements
			std::vector<CellPtr> cells_copy2(cells);
			TS_ASSERT_THROWS_NOTHING(VertexBasedCellPopulation<2> cell_population(*p_mesh, cells_copy2));

			// Create cell population
			VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

			// Check correspondence between elements and cells

			for (VertexMesh<2,2>::VertexElementIterator iter = p_mesh->GetElementIteratorBegin();
				 iter != p_mesh->GetElementIteratorEnd();
				 ++iter)
			{
				std::set<unsigned> expected_node_indices;
				unsigned expected_index = iter->GetIndex();

				for (unsigned i=0; i<iter->GetNumNodes(); i++)
				{
					expected_node_indices.insert(iter->GetNodeGlobalIndex(i));
				}

				std::set<unsigned> actual_node_indices;
				unsigned elem_index = iter->GetIndex();
				CellPtr p_cell = cell_population.GetCellUsingLocationIndex(elem_index);
				VertexElement<2,2>* p_actual_element = cell_population.GetElementCorrespondingToCell(p_cell);
				unsigned actual_index = p_actual_element->GetIndex();

				for (unsigned i=0; i<p_actual_element->GetNumNodes(); i++)
				{
					actual_node_indices.insert(p_actual_element->GetNodeGlobalIndex(i));
				}

				TS_ASSERT_EQUALS(actual_index, expected_index);
				TS_ASSERT_EQUALS(actual_node_indices, expected_node_indices);
			}
		}


    	{
			// Create a simple vertex-based mesh

			HoneycombMutableVertexMeshGenerator generator(3, 3);
			MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

			// Set up cells, one for each element.
			// Give each a birth time of -element_index, so the age = element_index.
			std::vector<CellPtr> cells;
			std::vector<unsigned> cell_location_indices;
			boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
			for (unsigned i=0; i<p_mesh->GetNumElements()+1; i++)
			{
				FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
				p_model->SetCellProliferativeType(STEM);

				CellPtr p_cell(new Cell(p_state, p_model));

				double birth_time = 0.0 - i;
				p_cell->SetBirthTime(birth_time);

				cells.push_back(p_cell);
				cell_location_indices.push_back(i%p_mesh->GetNumElements());		// Element 0 will have 2 cells
			}

			// This should throw an exception as the number of cells
			// does not equal the number of elements
			TS_ASSERT_THROWS_THIS(VertexBasedCellPopulation<2> cell_population(*p_mesh, cells, false, true, cell_location_indices),
					"Element 0 appears to have 2 cells associated with it");
		}
    }


    void TestGetDampingConstant()
    {
        CellBasedConfig::Instance()->SetDampingConstantMutant(8.0);

        // Create mesh
        HoneycombMutableVertexMeshGenerator generator(3, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells
        std::vector<CellPtr> cells;
        std::vector<unsigned> cell_location_indices;
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
        boost::shared_ptr<AbstractCellProperty> p_apc1(new ApcOneHitCellMutationState);
        boost::shared_ptr<AbstractCellProperty> p_apc2(new ApcTwoHitCellMutationState);
        boost::shared_ptr<AbstractCellProperty> p_bcat1(new BetaCateninOneHitCellMutationState);
        boost::shared_ptr<AbstractCellProperty> p_label(new CellLabel);

        for (unsigned i=0; i<p_mesh->GetNumElements(); i++)
        {
            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(STEM);

            CellPtr p_cell(new Cell(p_state, p_model));

            double birth_time = 0.0 - i;
            p_cell->SetBirthTime(birth_time);

            cells.push_back(p_cell);
            cell_location_indices.push_back(i);
        }
        cells[0]->SetMutationState(p_apc1);
        cells[6]->SetMutationState(p_apc2);
        cells[7]->SetMutationState(p_bcat1);
        cells[8]->AddCellProperty(p_label);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);
        cell_population.InitialiseCells(); // this method must be called explicitly as there is no simulation

        // Test GetDampingConstant()
        double normal_damping_constant = CellBasedConfig::Instance()->GetDampingConstantNormal();
        double mutant_damping_constant = CellBasedConfig::Instance()->GetDampingConstantMutant();

        // Node 0 is contained in cell 0 only, therefore should have mutant damping constant
        double damping_constant_at_node_0 = cell_population.GetDampingConstant(0);
        TS_ASSERT_DELTA(damping_constant_at_node_0, mutant_damping_constant, 1e-6);

        // Node 1 is contained in cell 2 only, therefore should have a normal damping constant
        double damping_constant_at_node_1 = cell_population.GetDampingConstant(1);
        TS_ASSERT_DELTA(damping_constant_at_node_1, normal_damping_constant, 1e-6);

        // Node 4 is contained in cells 0 and 1, therefore should an averaged damping constant
        double damping_constant_at_node_4 = cell_population.GetDampingConstant(4);
        TS_ASSERT_DELTA(damping_constant_at_node_4, (normal_damping_constant+mutant_damping_constant)/2.0, 1e-6);

        // Node 8 is contained in cells 0, 1, 3, therefore should an averaged damping constant
        double damping_constant_at_node_8 = cell_population.GetDampingConstant(8);
        TS_ASSERT_DELTA(damping_constant_at_node_8, (2*normal_damping_constant+mutant_damping_constant)/3.0, 1e-6);

        // Node 27 is contained in cell 6 only, therefore should have a mutant damping constant
        double damping_constant_at_node_27 = cell_population.GetDampingConstant(27);
        TS_ASSERT_DELTA(damping_constant_at_node_27, mutant_damping_constant, 1e-6);

        // Node 25 is contained in cells 6 and 7, therefore should have a mutant damping constant
        double damping_constant_at_node_25 = cell_population.GetDampingConstant(25);
        TS_ASSERT_DELTA(damping_constant_at_node_25, mutant_damping_constant, 1e-6);
    }


    void TestUpdateWithoutBirthOrDeath() throw (Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(10.0, 1);

        // Create a simple vertex-based mesh
        HoneycombMutableVertexMeshGenerator generator(4, 6);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        unsigned num_cells_removed = cell_population.RemoveDeadCells();
        TS_ASSERT_EQUALS(num_cells_removed, 0u);

        p_simulation_time->IncrementTimeOneStep();

        TS_ASSERT_THROWS_NOTHING(cell_population.Update());
    }


    void TestAddCellWithSimpleMesh() throw (Exception)
    {
        // Make some nodes
        std::vector<Node<2>*> nodes;
        nodes.push_back(new Node<2>(0, false, 2.0, -1.0));
        nodes.push_back(new Node<2>(1, false, 2.0, 1.0));
        nodes.push_back(new Node<2>(2, false, -2.0, 1.0));
        nodes.push_back(new Node<2>(3, false, -2.0, -1.0));
        nodes.push_back(new Node<2>(4, false, 0.0, 2.0));

        // Make a rectangular element out of nodes 0,1,2,3
        std::vector<Node<2>*> nodes_elem_1;
        nodes_elem_1.push_back(nodes[0]);
        nodes_elem_1.push_back(nodes[1]);
        nodes_elem_1.push_back(nodes[2]);
        nodes_elem_1.push_back(nodes[3]);

        // Make a triangular element out of nodes 1,4,2
        std::vector<Node<2>*> nodes_elem_2;
        nodes_elem_2.push_back(nodes[1]);
        nodes_elem_2.push_back(nodes[4]);
        nodes_elem_2.push_back(nodes[2]);

        std::vector<VertexElement<2,2>*> vertex_elements;
        vertex_elements.push_back(new VertexElement<2,2>(0, nodes_elem_1));
        vertex_elements.push_back(new VertexElement<2,2>(1, nodes_elem_2));

        // Make a vertex mesh
        MutableVertexMesh<2,2> vertex_mesh(nodes, vertex_elements);

        TS_ASSERT_EQUALS(vertex_mesh.GetNumElements(), 2u);
        TS_ASSERT_EQUALS(vertex_mesh.GetNumNodes(), 5u);

        // Create cells
        std::vector<CellPtr> cells = SetUpCells(vertex_mesh);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(vertex_mesh, cells);

        // For coverage, test GetLocationOfCellCentre()

        // Cell 0 is a rectangle with centre of mass (0,0)
        c_vector<double, 2> cell0_location = cell_population.GetLocationOfCellCentre(cell_population.GetCellUsingLocationIndex(0));
        TS_ASSERT_DELTA(cell0_location[0], 0.0, 1e-4);
        TS_ASSERT_DELTA(cell0_location[1], 0.0, 1e-4);

        // Cell 1 is a triangle with centre of mass (0,4/3)
        c_vector<double, 2> cell1_location = cell_population.GetLocationOfCellCentre(cell_population.GetCellUsingLocationIndex(1));
        TS_ASSERT_DELTA(cell1_location[0], 0.0, 1e-4);
        TS_ASSERT_DELTA(cell1_location[1], 4.0/3.0, 1e-4);

        unsigned old_num_nodes = vertex_mesh.GetNumNodes();
        unsigned old_num_elements = vertex_mesh.GetNumElements();
        unsigned old_num_cells = cell_population.rGetCells().size();

        // Add new cell by dividing element 0 along short axis

        c_vector<double,2> cell_division_vector = zero_vector<double>(2);

        CellPtr p_cell0 = cell_population.GetCellUsingLocationIndex(0);
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);

        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(STEM);
        CellPtr p_temp_cell(new Cell(p_state, p_model));
        p_temp_cell->SetBirthTime(-1);

        CellPtr p_new_cell = cell_population.AddCell(p_temp_cell, cell_division_vector, p_cell0);

        // Check that the new cell was successfully added to the cell population
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), old_num_nodes+2);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), old_num_elements+1);
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), old_num_cells+1);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), old_num_elements+1);

        // Check the location of the new nodes
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes)->rGetLocation()[0], 0.0, 1e-12);
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes)->rGetLocation()[1], 1.0, 1e-12);

        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes+1)->rGetLocation()[0], 0.0, 1e-12);
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes+1)->rGetLocation()[1], -1.0, 1e-12);

        // Now test the nodes in each element
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNumNodes(), 4u);
        TS_ASSERT_EQUALS(cell_population.GetElement(1)->GetNumNodes(), 4u);
        TS_ASSERT_EQUALS(cell_population.GetElement(2)->GetNumNodes(), 4u);

        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(0), 0u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(1), 1u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(2), 5u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(3), 6u);

        TS_ASSERT_EQUALS(cell_population.GetElement(1)->GetNodeGlobalIndex(0), 1u);
        TS_ASSERT_EQUALS(cell_population.GetElement(1)->GetNodeGlobalIndex(1), 4u);
        TS_ASSERT_EQUALS(cell_population.GetElement(1)->GetNodeGlobalIndex(2), 2u);
        TS_ASSERT_EQUALS(cell_population.GetElement(1)->GetNodeGlobalIndex(3), 5u);

        TS_ASSERT_EQUALS(cell_population.GetElement(2)->GetNodeGlobalIndex(0), 5u);
        TS_ASSERT_EQUALS(cell_population.GetElement(2)->GetNodeGlobalIndex(1), 2u);
        TS_ASSERT_EQUALS(cell_population.GetElement(2)->GetNodeGlobalIndex(2), 3u);
        TS_ASSERT_EQUALS(cell_population.GetElement(2)->GetNodeGlobalIndex(3), 6u);

        // Test ownership of the new nodes
        std::set<unsigned> expected_elements_containing_node_5;
        expected_elements_containing_node_5.insert(0);
        expected_elements_containing_node_5.insert(1);
        expected_elements_containing_node_5.insert(2);

        TS_ASSERT_EQUALS(cell_population.GetNode(5)->rGetContainingElementIndices(), expected_elements_containing_node_5);

        std::set<unsigned> expected_elements_containing_node_6;
        expected_elements_containing_node_6.insert(0);
        expected_elements_containing_node_6.insert(2);

        TS_ASSERT_EQUALS(cell_population.GetNode(6)->rGetContainingElementIndices(), expected_elements_containing_node_6);

        // Check the index of the new cell
        TS_ASSERT_EQUALS(cell_population.GetLocationIndexUsingCell(p_new_cell), old_num_elements);
    }


    void TestAddCellWithGivenDivisionVector() throw (Exception)
    {
        // Make a vertex mesh consisting of a single square element
        std::vector<Node<2>*> nodes;
        nodes.push_back(new Node<2>(0, false, 0.0, 0.0));
        nodes.push_back(new Node<2>(1, false, 2.0, 0.0));
        nodes.push_back(new Node<2>(2, false, 2.0, 1.0));
        nodes.push_back(new Node<2>(3, false, 0.0, 1.0));

        std::vector<Node<2>*> nodes_elem;
        for (unsigned i=0; i<4; i++)
        {
            nodes_elem.push_back(nodes[i]);
        }
        std::vector<VertexElement<2,2>*> vertex_elements;
        vertex_elements.push_back(new VertexElement<2,2>(0, nodes_elem));

        MutableVertexMesh<2,2> vertex_mesh(nodes, vertex_elements);

        // Create a cell
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
        FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
        p_model->SetCellProliferativeType(DIFFERENTIATED);

        CellPtr p_cell(new Cell(p_state, p_model));
        p_cell->SetBirthTime(-20.0);
        cells.push_back(p_cell);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(vertex_mesh, cells);

        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), 4u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 1u);
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), 1u);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), 1u);

        // Add a new cell by dividing element 0 along the axis (1,0)
        c_vector<double,2> cell_division_axis;
        cell_division_axis[0] = 1.0;
        cell_division_axis[1] = 0.0;

        FixedDurationGenerationBasedCellCycleModel* p_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model2->SetCellProliferativeType(STEM);
        CellPtr p_temp_cell(new Cell(p_state, p_model2));
        p_temp_cell->SetBirthTime(-1.0);

        CellPtr p_new_cell = cell_population.AddCell(p_temp_cell, cell_division_axis, p_cell);

        // Check that the new cell was successfully added to the cell population
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), 6u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 2u);
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), 2u);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), 2u);

        // Check the location of the new nodes
        TS_ASSERT_DELTA(cell_population.GetNode(4)->rGetLocation()[0], 2.0, 1e-12);
        TS_ASSERT_DELTA(cell_population.GetNode(4)->rGetLocation()[1], 0.5, 1e-12);
        TS_ASSERT_DELTA(cell_population.GetNode(5)->rGetLocation()[0], 0.0, 1e-12);
        TS_ASSERT_DELTA(cell_population.GetNode(5)->rGetLocation()[1], 0.5, 1e-12);

        // Test ownership of the new nodes
        std::set<unsigned> expected_elements_containing_node_4;
        expected_elements_containing_node_4.insert(0);
        expected_elements_containing_node_4.insert(1);
        TS_ASSERT_EQUALS(cell_population.GetNode(4)->rGetContainingElementIndices(), expected_elements_containing_node_4);

        std::set<unsigned> expected_elements_containing_node_5;
        expected_elements_containing_node_5.insert(0);
        expected_elements_containing_node_5.insert(1);
        TS_ASSERT_EQUALS(cell_population.GetNode(5)->rGetContainingElementIndices(), expected_elements_containing_node_5);

        // Check the index of the new cell
        TS_ASSERT_EQUALS(cell_population.GetLocationIndexUsingCell(p_new_cell), 1u);
    }

    void TestAddCellWithHoneycombMesh() throw (Exception)
    {
        // Create a mesh with 9 elements
        HoneycombMutableVertexMeshGenerator generator(3, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells, one for each VertexElement. Give each cell
        // a birth time of -elem_index, so its age is elem_index
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
        for (unsigned elem_index=0; elem_index<p_mesh->GetNumElements(); elem_index++)
        {
            CellProliferativeType cell_type = DIFFERENTIATED;
            double birth_time = 0.0 - elem_index;

            // Cell 4 should divide immediately
            if (elem_index==4)
            {
                cell_type = TRANSIT; // As stem cells always divide horizontally
                birth_time = -50.0;
            }

            FixedDurationGenerationBasedCellCycleModel* p_model = new FixedDurationGenerationBasedCellCycleModel();
            p_model->SetCellProliferativeType(cell_type);

            CellPtr p_cell(new Cell(p_state, p_model));
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Initialise cells (usually called in cell-based simulation constructor)
        cell_population.InitialiseCells();

        unsigned old_num_nodes = p_mesh->GetNumNodes();
        unsigned old_num_elements = p_mesh->GetNumElements();
        unsigned old_num_cells = cell_population.rGetCells().size();

        // Add a new cell by dividing cell 4

        cell_population.GetCellUsingLocationIndex(4)->ReadyToDivide();
        CellPtr p_cell4 = cell_population.GetCellUsingLocationIndex(4);

        CellPtr p_new_cell = cell_population.GetCellUsingLocationIndex(4)->Divide();

        c_vector<double, 2> new_location = zero_vector<double>(2);

        // Add new cell to the cell population
        cell_population.AddCell(p_new_cell, new_location, p_cell4);

        // Check that the new cell was successfully added to the cell population
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), old_num_nodes+2);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), old_num_elements+1);
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), old_num_cells+1);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), old_num_elements+1);

        // Check the location of the new nodes
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes)->rGetLocation()[0], 2.4721, 1e-4);
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes)->rGetLocation()[1], 1.7481, 1e-4);

        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes+1)->rGetLocation()[0], 1.5278, 1e-4);
        TS_ASSERT_DELTA(cell_population.GetNode(old_num_nodes+1)->rGetLocation()[1], 1.1386, 1e-4);

        // Now test the nodes in each element
        for (unsigned i=0; i<cell_population.GetNumElements(); i++)
        {
            if (i==4 || i==9)
            {
                // Elements 4 and 9 should each have one less node
                TS_ASSERT_EQUALS(cell_population.GetElement(i)->GetNumNodes(), 5u);
            }
            else if (i==1 || i==8)
            {
                // Elements 3 and 8 should each have one extra node
                TS_ASSERT_EQUALS(cell_population.GetElement(i)->GetNumNodes(), 7u);
            }
            else
            {
                TS_ASSERT_EQUALS(cell_population.GetElement(i)->GetNumNodes(), 6u);
            }
        }

        // Check node ownership for a few elements

        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(0), 0u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(1), 4u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(2), 8u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(3), 11u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(4), 7u);
        TS_ASSERT_EQUALS(cell_population.GetElement(0)->GetNodeGlobalIndex(5), 3u);

        TS_ASSERT_EQUALS(cell_population.GetElement(4)->GetNodeGlobalIndex(0), 9u);
        TS_ASSERT_EQUALS(cell_population.GetElement(4)->GetNodeGlobalIndex(1), 13u);
        TS_ASSERT_EQUALS(cell_population.GetElement(4)->GetNodeGlobalIndex(2), 17u);
        TS_ASSERT_EQUALS(cell_population.GetElement(4)->GetNodeGlobalIndex(3), 30u);
        TS_ASSERT_EQUALS(cell_population.GetElement(4)->GetNodeGlobalIndex(4), 31u);

        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(0), 17u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(1), 22u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(2), 26u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(3), 29u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(4), 25u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(5), 21u);
        TS_ASSERT_EQUALS(cell_population.GetElement(8)->GetNodeGlobalIndex(6), 30u);

        // Test element ownership for a few nodes

        std::set<unsigned> expected_elements_containing_node_5;
        expected_elements_containing_node_5.insert(1);
        expected_elements_containing_node_5.insert(2);

        TS_ASSERT_EQUALS(cell_population.GetNode(5)->rGetContainingElementIndices(), expected_elements_containing_node_5);

        std::set<unsigned> expected_elements_containing_node_13;
        expected_elements_containing_node_13.insert(2);
        expected_elements_containing_node_13.insert(4);
        expected_elements_containing_node_13.insert(5);

        TS_ASSERT_EQUALS(cell_population.GetNode(13)->rGetContainingElementIndices(), expected_elements_containing_node_13);

        std::set<unsigned> expected_elements_containing_node_30;
        expected_elements_containing_node_30.insert(8);
        expected_elements_containing_node_30.insert(4);
        expected_elements_containing_node_30.insert(9);

        TS_ASSERT_EQUALS(cell_population.GetNode(30)->rGetContainingElementIndices(), expected_elements_containing_node_30);
    }

    void TestIsCellAssociatedWithADeletedLocation() throw (Exception)
    {
        // Create a simple vertex-based mesh
        HoneycombMutableVertexMeshGenerator generator(4, 6);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();
        p_mesh->GetElement(5)->MarkAsDeleted();

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);

        // Create cell population but do not try to validate
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells, false, false);

        // Test IsCellAssociatedWithADeletedLocation() method
        for (VertexBasedCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            bool is_deleted = cell_population.IsCellAssociatedWithADeletedLocation(*cell_iter);

            if (cell_population.GetLocationIndexUsingCell(*cell_iter) == 5)
            {
                TS_ASSERT_EQUALS(is_deleted, true);
            }
            else
            {
                TS_ASSERT_EQUALS(is_deleted, false);
            }
        }
    }

    void TestRemoveDeadCellsAndUpdate() throw (Exception)
    {
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(10.0, 1);

        // Create a simple vertex-based mesh
        HoneycombMutableVertexMeshGenerator generator(4, 6);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);
        cells[5]->StartApoptosis();

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        TS_ASSERT_EQUALS(p_mesh->GetNumElements(), 24u);
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), 24u);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), 24u);
        TS_ASSERT_EQUALS(p_mesh->GetNumNodes(), 68u);

        p_simulation_time->IncrementTimeOneStep();

        // Remove dead cells
        unsigned num_cells_removed = cell_population.RemoveDeadCells();

        TS_ASSERT_EQUALS(num_cells_removed, 1u);

        // We should now have one less real cell, since one cell has been
        // marked as dead, so is skipped by the cell population iterator
        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), 68u);

        cell_population.Update();

        TS_ASSERT_EQUALS(cell_population.rGetCells().size(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumRealCells(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumElements(), 23u);
        TS_ASSERT_EQUALS(cell_population.GetNumNodes(), 68u);

        // Finally, check the cells' element indices have updated

        // We expect the cell element indices to be {0,11,...,23}
        std::set<unsigned> expected_elem_indices;
        for (unsigned i=0; i<cell_population.GetNumRealCells(); i++)
        {
            expected_elem_indices.insert(i);
        }

        // Get actual cell element indices
        std::set<unsigned> element_indices;

        for (AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            // Record element index corresponding to cell
            unsigned element_index = cell_population.GetLocationIndexUsingCell(*cell_iter);
            element_indices.insert(element_index);
        }

        TS_ASSERT_EQUALS(element_indices, expected_elem_indices);
    }


    void TestVertexBasedCellPopulationOutputWriters() throw (Exception)
    {
        // Set up SimulationTime (needed if VTK is used)
        SimulationTime::Instance()->SetEndTimeAndNumberOfTimeSteps(1.0, 1);

        // Create a simple vertex-based mesh
        HoneycombMutableVertexMeshGenerator generator(4, 6);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        TS_ASSERT_EQUALS(cell_population.GetIdentifier(), "VertexBasedCellPopulation-2");

        // For coverage of WriteResultsToFiles()
        boost::shared_ptr<AbstractCellProperty> p_state(cell_population.GetCellPropertyRegistry()->Get<WildTypeCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_apc1(cell_population.GetCellPropertyRegistry()->Get<ApcOneHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_apc2(cell_population.GetCellPropertyRegistry()->Get<ApcTwoHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_bcat1(cell_population.GetCellPropertyRegistry()->Get<BetaCateninOneHitCellMutationState>());
        boost::shared_ptr<AbstractCellProperty> p_apoptotic_state(cell_population.GetCellPropertyRegistry()->Get<ApoptoticCellProperty>());
        boost::shared_ptr<AbstractCellProperty> p_label(cell_population.GetCellPropertyRegistry()->Get<CellLabel>());

        cell_population.GetCellUsingLocationIndex(0)->GetCellCycleModel()->SetCellProliferativeType(TRANSIT);
        cell_population.GetCellUsingLocationIndex(0)->AddCellProperty(p_label);
        cell_population.GetCellUsingLocationIndex(1)->GetCellCycleModel()->SetCellProliferativeType(DIFFERENTIATED);
        cell_population.GetCellUsingLocationIndex(1)->SetMutationState(p_apc1);
        cell_population.GetCellUsingLocationIndex(2)->SetMutationState(p_apc2);
        cell_population.GetCellUsingLocationIndex(3)->SetMutationState(p_bcat1);
        cell_population.GetCellUsingLocationIndex(4)->AddCellProperty(p_apoptotic_state);
        cell_population.GetCellUsingLocationIndex(5)->GetCellCycleModel()->SetCellProliferativeType(STEM);
        cell_population.SetCellAncestorsToLocationIndices();

        cell_population.SetOutputCellMutationStates(true);
        cell_population.SetOutputCellProliferativeTypes(true);
        cell_population.SetOutputCellCyclePhases(true);
        cell_population.SetOutputCellAncestors(true);
        cell_population.SetOutputCellAges(true);
        cell_population.SetOutputCellVolumes(true);

        // Coverage of writing CellwiseData to VTK
        CellwiseData<2>* p_data = CellwiseData<2>::Instance();
        p_data->SetNumCellsAndVars(cell_population.GetNumRealCells(), 2);
        p_data->SetCellPopulation(&cell_population);
        for (unsigned var=0; var<2; var++)
        {
            for (AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
                 cell_iter != cell_population.End();
                 ++cell_iter)
            {
                p_data->SetValue((double) 3.0*var, cell_population.GetLocationIndexUsingCell(*cell_iter), var);
            }
        }

        std::string output_directory = "TestVertexBasedCellPopulationOutputWriters";
        OutputFileHandler output_file_handler(output_directory, false);

        TS_ASSERT_THROWS_NOTHING(cell_population.CreateOutputFiles(output_directory, false));

        cell_population.WriteResultsToFiles();

        TS_ASSERT_THROWS_NOTHING(cell_population.CloseOutputFiles());

        // Compare output with saved files of what they should look like
        std::string results_dir = output_file_handler.GetOutputDirectoryFullPath();

        TS_ASSERT_EQUALS(system(("diff " + results_dir + "results.viznodes     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/results.viznodes").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "results.vizelements     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/results.vizelements").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "results.vizcelltypes     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/results.vizcelltypes").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "results.vizancestors     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/results.vizancestors").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "cellmutationstates.dat     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/cellmutationstates.dat").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "cellages.dat     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/cellages.dat").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "cellcyclephases.dat     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/cellcyclephases.dat").c_str()), 0);
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "celltypes.dat     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/celltypes.dat").c_str()), 0);

        // Test the GetCellMutationStateCount function
        std::vector<unsigned> cell_mutation_states = cell_population.GetCellMutationStateCount();
        TS_ASSERT_EQUALS(cell_mutation_states.size(), 4u);
        TS_ASSERT_EQUALS(cell_mutation_states[0], 21u);
        TS_ASSERT_EQUALS(cell_mutation_states[1], 1u);
        TS_ASSERT_EQUALS(cell_mutation_states[2], 1u);
        TS_ASSERT_EQUALS(cell_mutation_states[3], 1u);

        // Test the GetCellProliferativeTypeCount function
        std::vector<unsigned> cell_types = cell_population.rGetCellProliferativeTypeCount();
        TS_ASSERT_EQUALS(cell_types.size(), 3u);
        TS_ASSERT_EQUALS(cell_types[0], 1u);
        TS_ASSERT_EQUALS(cell_types[1], 1u);
        TS_ASSERT_EQUALS(cell_types[2], 22u);

        // For coverage
        TS_ASSERT_THROWS_NOTHING(cell_population.WriteResultsToFiles());

        // Tidy up
        CellwiseData<2>::Destroy();

        //Test that the cell population parameters are output correctly
        out_stream parameter_file = output_file_handler.OpenOutputFile("results.parameters");

        // Write cell population parameters to file
        cell_population.OutputCellPopulationParameters(parameter_file);
        parameter_file->close();

        // Compare output with saved files of what they should look like
        TS_ASSERT_EQUALS(system(("diff " + results_dir + "results.parameters     notforrelease_cell_based/test/data/TestVertexBasedCellPopulationOutputWriters/results.parameters").c_str()), 0);

    }


    void TestArchiving2dVertexBasedCellPopulation() throw(Exception)
    {
        FileFinder archive_dir("archive", RelativeTo::ChasteTestOutput);
        std::string archive_file = "vertex_cell_population_2d.arch";
        // The following line is required because the loading of a cell population
        // is usually called by the method CellBasedSimulation::Load()
        ArchiveLocationInfo::SetMeshFilename("vertex_mesh_2d");

        // Create mesh
        VertexMeshReader<2,2> mesh_reader("mesh/test/data/TestVertexMeshWriter/vertex_mesh_2d");
        MutableVertexMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        // Archive cell population
        {
            // Need to set up time
            unsigned num_steps = 10;
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);

            // Set up cells
            std::vector<CellPtr> cells = SetUpCells(mesh);

            // Create cell population
            VertexBasedCellPopulation<2>* const p_cell_population = new VertexBasedCellPopulation<2>(mesh, cells);

            // Cells have been given birth times of 0 and -1.
            // Loop over them to run to time 0.0;
            for (AbstractCellPopulation<2>::Iterator cell_iter = p_cell_population->Begin();
                 cell_iter != p_cell_population->End();
                 ++cell_iter)
            {
                cell_iter->ReadyToDivide();
            }

            // Create output archive
            ArchiveOpener<boost::archive::text_oarchive, std::ofstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_oarchive* p_arch = arch_opener.GetCommonArchive();

            // Write the cell population to the archive
            (*p_arch) << static_cast<const SimulationTime&> (*p_simulation_time);
            (*p_arch) << p_cell_population;

            // Tidy up
            SimulationTime::Destroy();
            delete p_cell_population;
        }

        // Restore cell population
        {
            // Need to set up time
            unsigned num_steps = 10;
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);
            p_simulation_time->IncrementTimeOneStep();

            // Create an input archive
            ArchiveOpener<boost::archive::text_iarchive, std::ifstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_iarchive* p_arch = arch_opener.GetCommonArchive();

            // Restore the cell population
            (*p_arch) >> *p_simulation_time;
            VertexBasedCellPopulation<2>* p_cell_population;
            (*p_arch) >> p_cell_population;

            // Check the cell population has been restored correctly
            TS_ASSERT_EQUALS(p_cell_population->rGetCells().size(), 2u);

            // Cells have been given birth times of 0, -1, -2, -3, -4.
            // this checks that individual cells and their models are archived.
            unsigned counter = 0;
            for (AbstractCellPopulation<2>::Iterator cell_iter = p_cell_population->Begin();
                 cell_iter != p_cell_population->End();
                 ++cell_iter)
            {
                TS_ASSERT_DELTA(cell_iter->GetAge(), (double)(counter), 1e-7);
                counter++;
            }

            // Check the simulation time has been restored (through the cell)
            TS_ASSERT_EQUALS(p_simulation_time->GetTime(), 0.0);

            // Check the mesh has been restored correctly
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumNodes(), 7u);
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumElements(), 2u);
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumFaces(), 0u);

            // Compare the loaded mesh against the original
            MutableVertexMesh<2,2>& loaded_mesh = p_cell_population->rGetMesh();

            TS_ASSERT_EQUALS(mesh.GetNumNodes(), loaded_mesh.GetNumNodes());

            for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
            {
                Node<2>* p_node = mesh.GetNode(node_index);
                Node<2>* p_node2 = loaded_mesh.GetNode(node_index);

                TS_ASSERT_EQUALS(p_node->IsDeleted(), p_node2->IsDeleted());
                TS_ASSERT_EQUALS(p_node->GetIndex(), p_node2->GetIndex());

                TS_ASSERT_EQUALS(p_node->IsBoundaryNode(), p_node2->IsBoundaryNode());

                for (unsigned dimension=0; dimension<2; dimension++)
                {
                    TS_ASSERT_DELTA(p_node->rGetLocation()[dimension], p_node2->rGetLocation()[dimension], 1e-4);
                }
            }

            TS_ASSERT_EQUALS(mesh.GetNumElements(), loaded_mesh.GetNumElements());

            for (unsigned elem_index=0; elem_index < mesh.GetNumElements(); elem_index++)
            {
                TS_ASSERT_EQUALS(mesh.GetElement(elem_index)->GetNumNodes(),
                                 loaded_mesh.GetElement(elem_index)->GetNumNodes());

                for (unsigned local_index=0; local_index<mesh.GetElement(elem_index)->GetNumNodes(); local_index++)
                {
                    TS_ASSERT_EQUALS(mesh.GetElement(elem_index)->GetNodeGlobalIndex(local_index),
                                     loaded_mesh.GetElement(elem_index)->GetNodeGlobalIndex(local_index));
                }
            }

            // Tidy up
            delete p_cell_population;
        }
    }


    void TestArchiving3dVertexBasedCellPopulation() throw(Exception)
    {
        // Create mutable vertex mesh
        std::vector<Node<3>*> nodes;
        nodes.push_back(new Node<3>(0, false, 0.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(1, false, 1.0, 0.0, 0.0));
        nodes.push_back(new Node<3>(2, false, 0.0, 1.0, 0.0));
        nodes.push_back(new Node<3>(3, false, 0.0, 0.0, 1.0));
        nodes.push_back(new Node<3>(4, false, 1.0, 1.0, 0.0));
        nodes.push_back(new Node<3>(5, false, 0.0, 1.0, 1.0));
        nodes.push_back(new Node<3>(6, false, 1.0, 0.0, 1.0));
        nodes.push_back(new Node<3>(7, false, 1.0, 1.0, 1.0));
        nodes.push_back(new Node<3>(8, false, 0.5, 0.5, 1.5));

        std::vector<std::vector<Node<3>*> > nodes_faces(10);
        nodes_faces[0].push_back(nodes[0]);
        nodes_faces[0].push_back(nodes[2]);
        nodes_faces[0].push_back(nodes[4]);
        nodes_faces[0].push_back(nodes[1]);
        nodes_faces[1].push_back(nodes[4]);
        nodes_faces[1].push_back(nodes[7]);
        nodes_faces[1].push_back(nodes[5]);
        nodes_faces[1].push_back(nodes[2]);
        nodes_faces[2].push_back(nodes[7]);
        nodes_faces[2].push_back(nodes[6]);
        nodes_faces[2].push_back(nodes[1]);
        nodes_faces[2].push_back(nodes[4]);
        nodes_faces[3].push_back(nodes[0]);
        nodes_faces[3].push_back(nodes[3]);
        nodes_faces[3].push_back(nodes[5]);
        nodes_faces[3].push_back(nodes[2]);
        nodes_faces[4].push_back(nodes[1]);
        nodes_faces[4].push_back(nodes[6]);
        nodes_faces[4].push_back(nodes[3]);
        nodes_faces[4].push_back(nodes[0]);
        nodes_faces[5].push_back(nodes[7]);
        nodes_faces[5].push_back(nodes[6]);
        nodes_faces[5].push_back(nodes[3]);
        nodes_faces[5].push_back(nodes[5]);
        nodes_faces[6].push_back(nodes[6]);
        nodes_faces[6].push_back(nodes[7]);
        nodes_faces[6].push_back(nodes[8]);
        nodes_faces[7].push_back(nodes[6]);
        nodes_faces[7].push_back(nodes[8]);
        nodes_faces[7].push_back(nodes[3]);
        nodes_faces[8].push_back(nodes[3]);
        nodes_faces[8].push_back(nodes[8]);
        nodes_faces[8].push_back(nodes[5]);
        nodes_faces[9].push_back(nodes[5]);
        nodes_faces[9].push_back(nodes[8]);
        nodes_faces[9].push_back(nodes[7]);

        std::vector<VertexElement<2,3>*> faces;
        for (unsigned i=0; i<10; i++)
        {
            faces.push_back(new VertexElement<2,3>(i, nodes_faces[i]));
        }

        std::vector<VertexElement<2,3>*> faces_element_0, faces_element_1;
        std::vector<bool> orientations_0, orientations_1;
        for (unsigned i=0; i<6; i++)
        {
            faces_element_0.push_back(faces[i]);
            orientations_0.push_back(true);
        }
        for (unsigned i=6; i<10; i++)
        {
            faces_element_1.push_back(faces[i]);
            orientations_1.push_back(true);
        }
        faces_element_1.push_back(faces[5]);
        orientations_1.push_back(false);

        std::vector<VertexElement<3,3>*> elements;
        elements.push_back(new VertexElement<3,3>(0, faces_element_0, orientations_0));
        elements.push_back(new VertexElement<3,3>(1, faces_element_1, orientations_1));

        MutableVertexMesh<3,3> mesh(nodes, elements);

        FileFinder archive_dir("archive", RelativeTo::ChasteTestOutput);
        std::string archive_file = "vertex_cell_population_3d.arch";
        // The following line is required because the loading of a cell population
        // is usually called by the method CellBasedSimulation::Load()
        ArchiveLocationInfo::SetMeshFilename("vertex_mesh");

        // Archive cell population
        {
            // Need to set up time
            unsigned num_steps = 10;
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);

            // Set up cells
            std::vector<CellPtr> cells = SetUpCells(mesh);

            // Create cell population
            VertexBasedCellPopulation<3>* const p_cell_population = new VertexBasedCellPopulation<3>(mesh, cells);

            // Cells have been given birth times of 0 and -1.
            // Loop over them to run to time 0.0;
            for (AbstractCellPopulation<3>::Iterator cell_iter = p_cell_population->Begin();
                 cell_iter != p_cell_population->End();
                 ++cell_iter)
            {
                cell_iter->ReadyToDivide();
            }

            // Create output archive
            ArchiveOpener<boost::archive::text_oarchive, std::ofstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_oarchive* p_arch = arch_opener.GetCommonArchive();

            // Write the cell population to the archive
            (*p_arch) << static_cast<const SimulationTime&> (*p_simulation_time);
            (*p_arch) << p_cell_population;

            // Tidy up
            SimulationTime::Destroy();
            delete p_cell_population;
        }

        // Restore cell population
        {
            // Need to set up time
            unsigned num_steps = 10;
            SimulationTime* p_simulation_time = SimulationTime::Instance();
            p_simulation_time->SetStartTime(0.0);
            p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);
            p_simulation_time->IncrementTimeOneStep();

            // Create an input archive
            ArchiveOpener<boost::archive::text_iarchive, std::ifstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_iarchive* p_arch = arch_opener.GetCommonArchive();

            // Restore the cell population
            (*p_arch) >> *p_simulation_time;
            VertexBasedCellPopulation<3>* p_cell_population;
            (*p_arch) >> p_cell_population;

            // Check the cell population has been restored correctly
            TS_ASSERT_EQUALS(p_cell_population->rGetCells().size(), 2u);

            // Cells have been given birth times of 0, -1, -2, -3, -4.
            // this checks that individual cells and their models are archived.
            unsigned counter = 0;
            for (AbstractCellPopulation<3>::Iterator cell_iter = p_cell_population->Begin();
                 cell_iter != p_cell_population->End();
                 ++cell_iter)
            {
                TS_ASSERT_DELTA(cell_iter->GetAge(), (double)(counter), 1e-7);
                counter++;
            }

            // Check the simulation time has been restored (through the cell)
            TS_ASSERT_EQUALS(p_simulation_time->GetTime(), 0.0);

            // Check the mesh has been restored correctly
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumNodes(), 9u);
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumElements(), 2u);
            TS_ASSERT_EQUALS(p_cell_population->rGetMesh().GetNumFaces(), 10u);

            // Compare the loaded mesh against the original
            MutableVertexMesh<3,3>& loaded_mesh = p_cell_population->rGetMesh();

            TS_ASSERT_EQUALS(mesh.GetNumNodes(), loaded_mesh.GetNumNodes());

            for (unsigned node_index=0; node_index<mesh.GetNumNodes(); node_index++)
            {
                Node<3>* p_node = mesh.GetNode(node_index);
                Node<3>* p_node2 = loaded_mesh.GetNode(node_index);

                TS_ASSERT_EQUALS(p_node->IsDeleted(), p_node2->IsDeleted());
                TS_ASSERT_EQUALS(p_node->GetIndex(), p_node2->GetIndex());

                TS_ASSERT_EQUALS(p_node->IsBoundaryNode(), p_node2->IsBoundaryNode());

                for (unsigned dimension=0; dimension<2; dimension++)
                {
                    TS_ASSERT_DELTA(p_node->rGetLocation()[dimension], p_node2->rGetLocation()[dimension], 1e-4);
                }
            }

            TS_ASSERT_EQUALS(mesh.GetNumElements(), loaded_mesh.GetNumElements());

            for (unsigned elem_index=0; elem_index < mesh.GetNumElements(); elem_index++)
            {
                TS_ASSERT_EQUALS(mesh.GetElement(elem_index)->GetNumNodes(),
                                 loaded_mesh.GetElement(elem_index)->GetNumNodes());

                for (unsigned local_index=0; local_index<mesh.GetElement(elem_index)->GetNumNodes(); local_index++)
                {
                    TS_ASSERT_EQUALS(mesh.GetElement(elem_index)->GetNodeGlobalIndex(local_index),
                                     loaded_mesh.GetElement(elem_index)->GetNodeGlobalIndex(local_index));
                }
            }

            // Tidy up
            delete p_cell_population;
        }
    }


    void TestUpdateNodeLocations() throw (Exception)
    {
        // Create a simple 2D VertexMesh
        HoneycombMutableVertexMeshGenerator generator(5, 3);
        MutableVertexMesh<2,2>* p_mesh = generator.GetMutableMesh();

        // Impose a larger cell rearrangement threshold so that motion is uninhibited (see #1376)
        p_mesh->SetCellRearrangementThreshold(0.1);

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(*p_mesh);

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(*p_mesh, cells);

        // Make up some forces
        std::vector<c_vector<double, 2> > old_posns(cell_population.GetNumNodes());
        std::vector<c_vector<double, 2> > forces_on_nodes(cell_population.GetNumNodes());

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
            old_posns[i][0] = p_mesh->GetNode(i)->rGetLocation()[0];
            old_posns[i][1] = p_mesh->GetNode(i)->rGetLocation()[1];

            forces_on_nodes[i][0] = i*0.01;
            forces_on_nodes[i][1] = 2*i*0.01;
        }

        double time_step = 0.01;

        cell_population.UpdateNodeLocations(forces_on_nodes, time_step);

        for (unsigned i=0; i<cell_population.GetNumNodes(); i++)
        {
            TS_ASSERT_DELTA(cell_population.GetNode(i)->rGetLocation()[0], old_posns[i][0] +   i*0.01*0.01, 1e-9);
            TS_ASSERT_DELTA(cell_population.GetNode(i)->rGetLocation()[1], old_posns[i][1] + 2*i*0.01*0.01, 1e-9);
        }
    }


    /**
     * Test that post-#878, WntConcentration copes with a VertexBasedCellPopulation.
     * \todo When vertex-based cell population code is added to cell_based folder, move this
     *       test to TestWntConcentration.hpp
     */
    void TestWntConcentrationWithVertexBasedCellPopulation() throw(Exception)
    {
        // Make some nodes
        std::vector<Node<2>*> nodes;
        nodes.push_back(new Node<2>(0, false, 2.0, -1.0));
        nodes.push_back(new Node<2>(1, false, 2.0, 1.0));
        nodes.push_back(new Node<2>(2, false, -2.0, 1.0));
        nodes.push_back(new Node<2>(3, false, -2.0, -1.0));
        nodes.push_back(new Node<2>(4, false, 0.0, 2.0));

        // Make a rectangular element out of nodes 0,1,2,3
        std::vector<Node<2>*> nodes_elem_1;
        nodes_elem_1.push_back(nodes[0]);
        nodes_elem_1.push_back(nodes[1]);
        nodes_elem_1.push_back(nodes[2]);
        nodes_elem_1.push_back(nodes[3]);

        // Make a triangular element out of nodes 1,4,2
        std::vector<Node<2>*> nodes_elem_2;
        nodes_elem_2.push_back(nodes[1]);
        nodes_elem_2.push_back(nodes[4]);
        nodes_elem_2.push_back(nodes[2]);

        std::vector<VertexElement<2,2>*> vertex_elements;
        vertex_elements.push_back(new VertexElement<2,2>(0, nodes_elem_1));
        vertex_elements.push_back(new VertexElement<2,2>(1, nodes_elem_2));

        // Make a vertex mesh
        MutableVertexMesh<2,2> vertex_mesh(nodes, vertex_elements);

        // Create cells
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
        for (unsigned i=0; i<vertex_mesh.GetNumElements(); i++)
        {
            WntCellCycleModel* p_cell_cycle_model = new WntCellCycleModel;
            p_cell_cycle_model->SetDimension(2);
            p_cell_cycle_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_cell_cycle_model));
            double birth_time = 0.0 - i;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(vertex_mesh, cells);

        // Set the top of this cell_population, for the purposes of computing the WntConcentration
        CellBasedConfig::Instance()->SetCryptLength(4.0);

        // Set up an instance of the WntConcentration singleton object
        WntConcentration<2>* p_wnt = WntConcentration<2>::Instance();
        TS_ASSERT_EQUALS(p_wnt->IsWntSetUp(), false);

        // Check that the singleton can be set up
        p_wnt->SetType(LINEAR);
        p_wnt->SetCellPopulation(cell_population);

        TS_ASSERT_EQUALS(p_wnt->IsWntSetUp(), true);

        // Check that the singleton can be destroyed then recreated
        WntConcentration<2>::Destroy();
        WntConcentration<2>::Instance()->SetType(NONE);
        WntConcentration<2>::Instance()->SetCellPopulation(cell_population);
        TS_ASSERT_EQUALS(WntConcentration<2>::Instance()->IsWntSetUp(), false); // not fully set up now it is a NONE type

        WntConcentration<2>::Destroy();
        WntConcentration<2>::Instance()->SetType(LINEAR);
        WntConcentration<2>::Instance()->SetCellPopulation(cell_population);

        p_wnt = WntConcentration<2>::Instance();
        TS_ASSERT_EQUALS(p_wnt->IsWntSetUp(), true); // set up again

        double wnt_at_cell0 = p_wnt->GetWntLevel(cell_population.GetCellUsingLocationIndex(0));
        double wnt_at_cell1 = p_wnt->GetWntLevel(cell_population.GetCellUsingLocationIndex(1));

        // We have set the top of the cell population to be 4, so the WntConcentration should decrease linearly
        // up the cell_population, from one at height 0 to zero at height 4.

        // Cell 0 has centre of mass (0,0)
        TS_ASSERT_DELTA(wnt_at_cell0, 1.0, 1e-4);

        // Cell 1 has centre of mass (0, 4/3)
        TS_ASSERT_DELTA(wnt_at_cell1, 2.0/3.0, 1e-4);
    }

    ///\todo When vertex models are released, move this test into TestCellwiseData (see also #1419)
    void TestCellwiseDataWithVertexBasedCellPopulation() throw(Exception)
    {
        // Make some nodes
        std::vector<Node<2>*> nodes;
        nodes.push_back(new Node<2>(0, false, 2.0, -1.0));
        nodes.push_back(new Node<2>(1, false, 2.0, 1.0));
        nodes.push_back(new Node<2>(2, false, -2.0, 1.0));
        nodes.push_back(new Node<2>(3, false, -2.0, -1.0));
        nodes.push_back(new Node<2>(4, false, 0.0, 2.0));

        // Make a rectangular element out of nodes 0,1,2,3
        std::vector<Node<2>*> nodes_elem_1;
        nodes_elem_1.push_back(nodes[0]);
        nodes_elem_1.push_back(nodes[1]);
        nodes_elem_1.push_back(nodes[2]);
        nodes_elem_1.push_back(nodes[3]);

        // Make a triangular element out of nodes 1,4,2
        std::vector<Node<2>*> nodes_elem_2;
        nodes_elem_2.push_back(nodes[1]);
        nodes_elem_2.push_back(nodes[4]);
        nodes_elem_2.push_back(nodes[2]);

        std::vector<VertexElement<2,2>*> vertex_elements;
        vertex_elements.push_back(new VertexElement<2,2>(0, nodes_elem_1));
        vertex_elements.push_back(new VertexElement<2,2>(1, nodes_elem_2));

        // Make a vertex mesh
        MutableVertexMesh<2,2> vertex_mesh(nodes, vertex_elements);

        // Create cells
        std::vector<CellPtr> cells;
        boost::shared_ptr<AbstractCellProperty> p_state(new WildTypeCellMutationState);
        for (unsigned i=0; i<vertex_mesh.GetNumElements(); i++)
        {
            WntCellCycleModel* p_cell_cycle_model = new WntCellCycleModel;
            p_cell_cycle_model->SetDimension(2);
            p_cell_cycle_model->SetCellProliferativeType(DIFFERENTIATED);

            CellPtr p_cell(new Cell(p_state, p_cell_cycle_model));
            double birth_time = 0.0 - i;
            p_cell->SetBirthTime(birth_time);
            cells.push_back(p_cell);
        }

        // Create cell population
        VertexBasedCellPopulation<2> cell_population(vertex_mesh, cells);

        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), false);

        // One variable tests

        CellwiseData<2>* p_data = CellwiseData<2>::Instance();

        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), false);
        TS_ASSERT_THROWS_THIS(p_data->SetCellPopulation(&cell_population),"SetCellPopulation must be called after SetNumCellsAndVars()");

        p_data->SetNumCellsAndVars(cell_population.GetNumRealCells(), 1);

        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), false);

        p_data->SetCellPopulation(&cell_population);

        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), true);

        p_data->SetValue(1.23, cell_population.GetElement(0)->GetIndex());
        AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
        TS_ASSERT_DELTA(p_data->GetValue(*cell_iter), 1.23, 1e-12);

        p_data->SetValue(2.23, cell_population.GetElement(1)->GetIndex());
        ++cell_iter;
        TS_ASSERT_DELTA(p_data->GetValue(*cell_iter), 2.23, 1e-12);

        // Test ReallocateMemory method

        // Add a new cell by dividing element 0 along the axis (1,0)
        c_vector<double,2> cell_division_axis;
        cell_division_axis[0] = 1.0;
        cell_division_axis[1] = 0.0;

        FixedDurationGenerationBasedCellCycleModel* p_model2 = new FixedDurationGenerationBasedCellCycleModel();
        p_model2->SetCellProliferativeType(STEM);
        CellPtr p_new_cell(new Cell(p_state, p_model2));
        p_new_cell->SetBirthTime(-1.0);

        cell_population.AddCell(p_new_cell, cell_division_axis, *cell_iter);

        TS_ASSERT_THROWS_NOTHING(p_data->ReallocateMemory());

        // Coverage
        std::vector<double> constant_value;
        constant_value.push_back(1.579);
        p_data->SetConstantDataForTesting(constant_value);

        for (AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
             cell_iter != cell_population.End();
             ++cell_iter)
        {
            TS_ASSERT_DELTA(p_data->GetValue(*cell_iter), 1.579, 1e-12);
        }

        p_data->Destroy();

        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), false);

        // Two variable tests

        p_data = CellwiseData<2>::Instance();

        p_data->SetNumCellsAndVars(cell_population.GetNumRealCells(), 2);
        p_data->SetCellPopulation(&cell_population);

        TS_ASSERT_THROWS_THIS(p_data->SetNumCellsAndVars(cell_population.GetNumRealCells(), 1),"SetNumCellsAndVars() must be called before setting the CellPopulation (and after a Destroy)");
        TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), true);

        p_data->SetValue(3.23, cell_population.GetElement(0)->GetIndex(), 1);
        AbstractCellPopulation<2>::Iterator cell_iter2 = cell_population.Begin();

        TS_ASSERT_DELTA(p_data->GetValue(*cell_iter2, 1), 3.23, 1e-12);

        p_data->SetValue(4.23, cell_population.GetElement(1)->GetIndex(), 1);
        ++cell_iter2;

        TS_ASSERT_DELTA(p_data->GetValue(*cell_iter2, 1), 4.23, 1e-12);

        // Other values should have been initialised to zero
        ++cell_iter2;
        TS_ASSERT_DELTA(p_data->GetValue(*cell_iter2, 0), 0.0, 1e-12);

        // Tidy up
        CellwiseData<2>::Destroy();
    }

    ///\todo When vertex models are released, move this test into TestCellwiseData (see also #1419)
    void TestArchiveCellwiseDataWithVertexBasedCellPopulation()
    {
        FileFinder archive_dir("archive", RelativeTo::ChasteTestOutput);
        std::string archive_file = "vertex_cellwise.arch";
        // The following line is required because the loading of a cell population
        // is usually called by the method CellBasedSimulation::Load()
        ArchiveLocationInfo::SetMeshFilename("vertex_cellwise");

        // Create mesh
        VertexMeshReader<2,2> mesh_reader("mesh/test/data/TestVertexMeshWriter/vertex_mesh_2d");
        MutableVertexMesh<2,2> mesh;
        mesh.ConstructFromMeshReader(mesh_reader);

        // Need to set up time
        unsigned num_steps = 10;
        SimulationTime* p_simulation_time = SimulationTime::Instance();
        p_simulation_time->SetEndTimeAndNumberOfTimeSteps(1.0, num_steps+1);

        // Set up cells
        std::vector<CellPtr> cells = SetUpCells(mesh);

        // Create cell population
        VertexBasedCellPopulation<2>* const p_cell_population = new VertexBasedCellPopulation<2>(mesh, cells);

        // Cells have been given birth times of 0 and -1.
        // Loop over them to run to time 0.0;
        for (AbstractCellPopulation<2>::Iterator cell_iter = p_cell_population->Begin();
             cell_iter != p_cell_population->End();
             ++cell_iter)
        {
            cell_iter->ReadyToDivide();
        }

        {
            // Set up the data store
            CellwiseData<2>* p_data = CellwiseData<2>::Instance();
            p_data->SetNumCellsAndVars(p_cell_population->GetNumRealCells(), 1);
            p_data->SetCellPopulation(p_cell_population);

            // Put some data in
            unsigned i = 0;
            for (AbstractCellPopulation<2>::Iterator cell_iter = p_cell_population->Begin();
                 cell_iter != p_cell_population->End();
                 ++cell_iter)
            {
                p_data->SetValue((double) i, p_cell_population->GetLocationIndexUsingCell(*cell_iter), 0);
                i++;
            }

            TS_ASSERT_EQUALS(p_data->IsSetUp(), true);

            // Create output archive
            ArchiveOpener<boost::archive::text_oarchive, std::ofstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_oarchive* p_arch = arch_opener.GetCommonArchive();

            // Write to the archive
            (*p_arch) << static_cast<const CellwiseData<2>&>(*CellwiseData<2>::Instance());

            CellwiseData<2>::Destroy();
            delete p_cell_population;
        }

        {
            CellwiseData<2>* p_data = CellwiseData<2>::Instance();

            // Create an input archive
            ArchiveOpener<boost::archive::text_iarchive, std::ifstream> arch_opener(archive_dir, archive_file);
            boost::archive::text_iarchive* p_arch = arch_opener.GetCommonArchive();

            (*p_arch) >> *p_data;

            // Check the data
            TS_ASSERT_EQUALS(CellwiseData<2>::Instance()->IsSetUp(), true);
            TS_ASSERT_EQUALS(p_data->IsSetUp(), true);

            // We will have constructed a new cell population on load, so use the new cell population
            AbstractCellPopulation<2>& cell_population = p_data->rGetCellPopulation();

            for (AbstractCellPopulation<2>::Iterator cell_iter = cell_population.Begin();
                 cell_iter != cell_population.End();
                 ++cell_iter)
            {
                TS_ASSERT_DELTA(p_data->GetValue(*cell_iter, 0u), (double) cell_population.GetLocationIndexUsingCell(*cell_iter), 1e-12);
            }

            // Tidy up
            CellwiseData<2>::Destroy();
            delete (&cell_population);
        }
    }
};

#endif /*TESTVERTEXBASEDCELLPOPULATION_HPP_*/