#ifndef LOOP_FILTER
#define LOOP_FILTER

#include <algorithm>
#include "graph_pack.hpp"
#include "omni/mf_ec_remover.hpp"

using omnigraph::StroglyConnectedComponentFinder;
using omnigraph::FlowGraph;

namespace debruijn_graph {
	
/*	template <class GraphPack>
	void filter_loops( const GraphPack& graph_p ) {

		StroglyConnectedComponentFinder<FlowGraph<typename GraphPack::graph_t>> finder(FlowGraph<typename GraphPack::graph_t>(graph_p.g));
		auto labeledVertices = finder.ColourComponents();
		
		std::map<size_t, std::set<EdgeId>> setLoops;
		for ( auto vIter = labeledVertices.begin(); vIter != labeledVertices.end(); ++vIter  ) {
		
			for ( auto eIter = graph_p.g.in_begin(*vIter); eIter != graph_p.g.in_end(); ++eIter ){
				
			//	auto vStart = graph_p.g.edgeStart( *eIter );
			//	if ( labeledVertices[*vIter] == labeledVertices[vStart] ){
				
					setLoops[ labeledVertices[*vIter] ].insert(*eIter); 
				
			//	}
			}
		
		
			for ( auto eIter = graph_p.g.out_begin(*vIter); eIter != graph_p.g.out_end(); ++eIter ){
				
			//	auto vEnd = graph_p.g.edgeEnd( *eIter );
			//	if ( labeledVertices[*vIter] == labeledVertices[vEnd] ){
				
					setLoops[ labeledVertices[*vIter] ].insert(*eIter); 
				
			//	}
			}

			
		}


	}
*/

	template <class GraphPack, class DetailedCoverage>
	struct LoopFilter {

		GraphPack* graph_p;
		DetailedCoverage* coverage;
		std::vector<VertexId> order;
		std::set<VertexId> usedVertices;
		std::vector< set<EdgeId> > simpleLoops;

		//contains all the vertices that go to complex loops
		std::set<EdgeId> prohibitedEdges;
		std::vector< std::vector<EdgeId> > resolvedLoops;

		LoopFilter ( GraphPack& g, DetailedCoverage& index ) {
	
			graph_p = &g;
			coverage = &index;
		}

		void get_loopy_components( /*EdgeQuality<typename GraphPack::graph_t>& quality_labeler*/ ) {

			for ( auto v = graph_p->g.begin(); v != graph_p->g.end(); ++v ) {
			
				if ( usedVertices.find(*v) != usedVertices.end() ) 
					continue;
			
				dfs1(*v);

			}

			usedVertices.clear();
			

			std::vector<std::vector<VertexId>> loops;
			for ( int i = 0; i < order.size(); ++i ) {
			
				VertexId startVertex = order[order.size() - i - 1];
				if ( usedVertices.find(startVertex) != usedVertices.end() ) 
					continue;
				std::vector<VertexId> loop;
				dfs2(startVertex, loop);
				loops.push_back(loop);
				
				
			}

			int L = 0;
			//std::vector<EdgeId> resolvedLoops;
			for (auto loop = loops.begin(); loop != loops.end(); ++loop ){

				/*for (auto v = loop->begin(); v != loop->end(); ++v ) {
					std::cout << graph_p->g.int_id(*v) << "  ";
				}*/
				if (loop->size() == 1) {
					continue;
				}
				std::set<EdgeId> path;
				std::vector<EdgeId> resolvedLoop;
				EdgeId incomingEdge, outgoingEdge;
				bool resolved = false;
				bool ifSimple = ifSimpleLoop(*loop, path, incomingEdge, outgoingEdge);

				std::cout << std::endl;
				if (ifSimple) {
					std::cout << "simple loop " ;
					if ( graph_p->g.int_id(incomingEdge) != 0 && graph_p->g.int_id(outgoingEdge) != 0 )
						resolved = resolveSimpleLoop( resolvedLoop, incomingEdge, outgoingEdge );
				}
				/*if (!resolved) {
					std::cout << "not simple loop or not resolved\n";
					prohibitedEdges.insert(path.begin(), path.end());
				}*/
				if (resolved) {
					for (auto e = resolvedLoop.begin(); e != resolvedLoop.end(); ++e ) {
						L += graph_p->g.length(*e);
					}
					resolvedLoops.push_back(resolvedLoop);
					//std::cout << "path: ";
				}
				else {
					//std::cout << "not path: ";
				}
				/*for (auto e = path.begin(); e != path.end(); ++e ) {
					std::cout <<  graph_p->g.int_id(graph_p->g.EdgeStart(*e)) << ", " << graph_p->g.int_id(graph_p->g.EdgeEnd(*e)) << ", ";
				}*/

				for (auto e = resolvedLoop.begin(); e != resolvedLoop.end(); ++e ) {
					std::cout << graph_p->g.int_id(*e) << " (cov: " << graph_p->g.coverage(*e) << ", q: " ;//<< quality_labeler.quality(*e) << ") ";
				}
				std::cout << std::endl << std::endl;
			}
			std::cout << "Overall length is " << L << std::endl;

		}	

		bool resolveSimpleLoop( std::vector<EdgeId>& resolvedLoop, 
					EdgeId& incomingEdge, EdgeId& outgoingEdge ) {

			EdgeId startEdge, endEdge;
				
			bool canBeResolved = true;

			VertexId inVertex = graph_p->g.EdgeEnd( incomingEdge );
			//VertexId outVertex = graph_p->g.EdgeStart( outgoingEdge );
			auto inCov = coverage->getOutCov(incomingEdge);
			auto outCov = coverage->getInCov(outgoingEdge);
			auto cov = (inCov + outCov) / 2.0;	
			//auto cov = inCov;
			//auto cov = min(inCov, outCov);	

			std::cout << "inCoverage: " << inCov << "; outCoverage " << outCov << "; cov: " << cov << std::endl;
			//INFO("after initialization");

			//INFO("before outgoing edges");
			auto loopEdge1 = *graph_p->g.OutgoingEdges(inVertex).begin();

			VertexId edge1End = graph_p->g.EdgeEnd(loopEdge1);
			EdgeId loopEdge2;
			auto loopEdge2it = graph_p->g.OutgoingEdges(edge1End).begin();
			if ( *loopEdge2it == outgoingEdge ) {
				++loopEdge2it;
			}
			loopEdge2 = *loopEdge2it;

			auto cov1 = graph_p->g.coverage(loopEdge1);
			auto cov2 = graph_p->g.coverage(loopEdge2);

			double intpart;
			double ratioThresholdLower = 0.3;
			double ratioThresholdUpper = 0.7;
			auto ratio1 = modf( cov1 / cov, &intpart );
			auto ratio2 = modf( cov2 / cov, &intpart );

			if ( ( ratio1 > ratioThresholdLower && ratio1 < ratioThresholdUpper ) || ( ratio2 > ratioThresholdLower && ratio2 < ratioThresholdUpper ) ) {

				canBeResolved = false;
			}
			auto time1 = floor( cov1 / cov + 0.5 );
			auto time2 = floor( cov2 / cov + 0.5 );

			std::cout << time1 << " " << time2 << std::endl;
			std::cout << time1 * cov << " " << cov1 << " " << time2 * cov << " " << cov2 << std::endl;
			double threshold = 0.75;
			if (time1 - time2 != 1 || time2 == 0) {
				canBeResolved = false;
			}

		/*	else if (  min(time1 * cov, cov1) / max(time1 * cov, cov1) < threshold   ||  min(time2 * cov,cov2) / max(time2 * cov,cov2) < threshold )  {

				canBeResolved = false;
			}
		*/

			if (canBeResolved) {
				
				resolvedLoop.push_back(incomingEdge);
				resolvedLoop.push_back(loopEdge1);

				for ( int i = 0; i < time2; ++i ) {
				
					resolvedLoop.push_back(loopEdge2);
					resolvedLoop.push_back(loopEdge1);
				}
				resolvedLoop.push_back(outgoingEdge);
				if ( resolvedLoop.size() > 5 ) canBeResolved = false;
			}

			return canBeResolved;
		}

		bool ifSimpleLoop( std::vector<VertexId>& loop, std::set<EdgeId>& path, EdgeId& incomingEdge, EdgeId& outgoingEdge ){
		//the idea: if a loop is simple there is a single possible way to come into any vertex in the component

			bool ifSimple = true;
			int nextVerticesOutOfLoop = 0;
			int prevVerticesOutOfLoop = 0;
		
			for ( auto v = loop.begin(); v != loop.end(); ++v ) {

				///std::cout << "vertex: " << graph_p->g.int_id(*v) << std::endl;
				auto incomingEdges = graph_p->g.IncomingEdges(*v);

				int prevVerticesInLoop = 0;
				//if ( incomingEdges.size() > 1 ) {
		
					///std::cout << "incoming edges: ";
					for ( auto e = incomingEdges.begin(); e != incomingEdges.end(); ++e ){
						///std::cout << graph_p->g.int_id(*e) << " ";
						auto startVertex = graph_p->g.EdgeStart(*e);
						
						// count the number of edges in the loop coming into this vertex	
						if ( std::find(loop.begin(),loop.end(),startVertex) != loop.end() ){
							prevVerticesInLoop+=1;
							if (prevVerticesInLoop > 1) {
								ifSimple = false;
							}
							path.insert(*e);
							//prohibitedEdges.insert(*e);
						}
						//count the number of edges coming into loop, i.e. check if the loop is a repeat
						else {
							prevVerticesOutOfLoop+=1;
							incomingEdge = *e;
							if (prevVerticesOutOfLoop > 1) {
								ifSimple = false;
							}
						}

					}
					///std::cout << std::endl;


				//}


				auto outgoingEdges = graph_p->g.OutgoingEdges(*v);

				int nextVerticesInLoop = 0;
				//if (outgoingEdges.size() > 1 ) {
					
					///std::cout << "outgoing edges: " ;
					for ( auto e = outgoingEdges.begin(); e != outgoingEdges.end(); ++e ){
						///std::cout << graph_p->g.int_id(*e) << " ";
						auto endVertex = graph_p->g.EdgeEnd(*e);
						// count the number of edges in the loop coming out of the vertex	
						if ( std::find(loop.begin(), loop.end(), endVertex) != loop.end() ){
							nextVerticesInLoop += 1;
							if (nextVerticesInLoop > 1){
								ifSimple = false;
							}
							path.insert(*e);
							//prohibitedEdges.insert(*e);
						}
						//count the number of edges coming into loop, i.e. check if the loop is a repeat
						else {
							nextVerticesOutOfLoop+=1;
							outgoingEdge = *e;
							if (nextVerticesOutOfLoop > 1) {
								ifSimple = false;
							}
						}

					}
					///std::cout << std::endl;

			//	}

			}

			
			/*for ( auto e = path.begin(); e != path.end(); ++e) {
				if ( graph_p->g.length(*e) > 1000) {
					return false;
				}
			}*/

			for ( auto e = path.begin(); e != path.end(); ++e) {
				prohibitedEdges.insert(*e);
			}
			return ifSimple;

		}

		/*void sort_path( std::vector<EdgeId&> path ){

			std::set<VertexId> vertices;

			for ( auto e = path.begin(); e != path.end(); ++e){
				vertices.insert(graph_p.g.EdgeStart(*e));
				vertices.insert(graph_p.g.EdgeEnd(*e));
			}
			
			auto e = path.begin(v);
			for ( ; vertices.find(graph_p.g.EdgeStart(*e)) != vertices.end() || path.end(); ++e){
				
			}
			if (e )
		}*/

		/*void  join_paths( std::vector<std::vector<EdgeId>>& paths ) {

			for ( auto loop = loops.begin(); loop != loops.end(); ++loop ) {

				std::vector<EdgeId> path;
				if ( ifSimpleLoop(loop, path) ){
					paths.push_back(path);
				}

			}

		}*/

		void dfs1( const VertexId& v ){

			usedVertices.insert(v);

			for ( auto incidentEdge = graph_p->g.out_begin(v); incidentEdge != graph_p->g.out_end(v); ++incidentEdge ){
	
				if ( graph_p->g.length(*incidentEdge) > 1000 ) continue;

				VertexId vOut = graph_p->g.EdgeEnd(*incidentEdge);
				if (usedVertices.find(vOut) != usedVertices.end())
					continue;
				dfs1( vOut );
			}

			order.push_back(v);

		}

		void dfs2( const VertexId& v, vector<VertexId>& loop ){

			usedVertices.insert( v );
			loop.push_back( v );

			for ( auto incidentEdge = graph_p->g.in_begin(v); incidentEdge != graph_p->g.in_end(v); ++incidentEdge ) {
				
				if ( graph_p->g.length(*incidentEdge) > 1000 ) continue;

				VertexId vIn = graph_p->g.EdgeStart(*incidentEdge);
				if (usedVertices.find(vIn) != usedVertices.end()) 
					continue;
				dfs2( vIn, loop );
			}


		}
	};
	
}

#endif
