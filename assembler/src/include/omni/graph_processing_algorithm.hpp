#pragma once

#include "logger/logger.hpp"
#include "func.hpp"
#include "graph_component.hpp"
#include "coverage.hpp"

namespace omnigraph {

template<class Graph>
class EdgeProcessingAlgorithm {
    typedef typename Graph::EdgeId EdgeId;
    typedef std::shared_ptr<func::Predicate<EdgeId>> ProceedConditionT;

    Graph& g_;
    bool conjugate_symmetry_;
 protected:

    Graph& g() {
        return g_;
    }

    const Graph& g() const {
        return g_;
    }

    virtual bool ProcessEdge(EdgeId e) = 0;

 public:
    EdgeProcessingAlgorithm(Graph& g, bool conjugate_symmetry = false)
            : g_(g), conjugate_symmetry_(conjugate_symmetry) {

    }

    virtual ~EdgeProcessingAlgorithm() {
    }

//    bool conjugate_symmetry() const {
//        return conjugate_symmetry_;
//    }

    template<class SmartEdgeIt>
    bool RunFromIterator(SmartEdgeIt& it,
                 ProceedConditionT proceed_condition = std::make_shared<func::AlwaysTrue<EdgeId>>()) {
        VERIFY(!it.canonical_only() || conjugate_symmetry_);
        TRACE("Start processing");
        bool triggered = false;
        for (; !it.IsEnd(); ++it) {
            EdgeId e = *it;
            TRACE("Current edge " << g_.str(e));
            if (!proceed_condition->Check(e)) {
                TRACE("Stop condition was reached.");
                //need to release last element of the iterator to make it replacable by new elements
                it.ReleaseCurrent();
                break;
            }

            TRACE("Processing edge " << this->g().str(e));
            triggered |= ProcessEdge(e);
        }
        TRACE("Finished processing. Triggered = " << triggered);
        return triggered;
    }

    template<class Comparator = std::less<EdgeId>>
    bool Run(const Comparator& comp = Comparator(),
                 ProceedConditionT proceed_condition = make_shared<func::AlwaysTrue<EdgeId>>()) {
        auto it = g_.SmartEdgeBegin(comp, conjugate_symmetry_);
        return RunFromIterator(it, proceed_condition);
    }

 private:
    DECL_LOGGER("EdgeProcessingAlgorithm");
};

template<class Graph>
void RemoveIsolatedOrCompress(Graph& g, typename Graph::VertexId v) {
    if (g.IsDeadStart(v) && g.IsDeadEnd(v)) {
        g.DeleteVertex(v);
    } else {
        g.CompressVertex(v);
    }
}

template<class Graph>
class EdgeRemover {
    typedef typename Graph::EdgeId EdgeId;
    typedef typename Graph::VertexId VertexId;
    typedef std::function<void(EdgeId)> HandlerF;

    Graph& g_;
    HandlerF removal_handler_;

 public:
    EdgeRemover(Graph& g, HandlerF removal_handler = nullptr)
            : g_(g),
              removal_handler_(removal_handler) {
    }

    //todo how is it even compiling with const?!!!
    void DeleteEdge(EdgeId e) {
        VertexId start = g_.EdgeStart(e);
        VertexId end = g_.EdgeEnd(e);
        DeleteEdgeWithNoCompression(e);
        // NOTE: e here is already dead!
        TRACE("Compressing locality");
        if (!g_.RelatedVertices(start, end)) {
            TRACE("Vertices not related");
            TRACE("Processing end");
            RemoveIsolatedOrCompress(g_, end);
            TRACE("End processed");
        }
        TRACE("Processing start");
        RemoveIsolatedOrCompress(g_, start);
        TRACE("Start processed");
    }

    void DeleteEdgeWithNoCompression(EdgeId e) {
        TRACE("Deletion of edge " << g_.str(e));
        TRACE("Start " << g_.str(g_.EdgeStart(e)));
        TRACE("End " << g_.str(g_.EdgeEnd(e)));
        if (removal_handler_) {
            TRACE("Calling handler");
            removal_handler_(e);
        }
        TRACE("Deleting edge");
        g_.DeleteEdge(e);
    }

 private:
    DECL_LOGGER("EdgeRemover");
};

template<class Graph>
class EdgeRemovingAlgorithm : public EdgeProcessingAlgorithm<Graph> {
    typedef EdgeProcessingAlgorithm<Graph> base;
    typedef typename Graph::EdgeId EdgeId;

    shared_ptr<func::Predicate<EdgeId>> remove_condition_;
    EdgeRemover<Graph> edge_remover_;

 protected:
    bool ProcessEdge(EdgeId e) {
        TRACE("Checking edge " << this->g().str(e) << " for the removal condition");
        if (remove_condition_->Check(e)) {
            TRACE("Check passed, removing");
            edge_remover_.DeleteEdge(e);
            return true;
        }
        TRACE("Check not passed");
        return false;
    }

 public:
    EdgeRemovingAlgorithm(
            Graph& g,
            shared_ptr<func::Predicate<EdgeId>> remove_condition,
            std::function<void (EdgeId)> removal_handler = boost::none,
            bool conjugate_symmetry = false)
            : base(g, conjugate_symmetry),
              remove_condition_(remove_condition),
              edge_remover_(g, removal_handler) {

    }

 private:
    DECL_LOGGER("EdgeRemovingAlgorithm");
};

//todo rewrite with SmartSetIterator
template<class Graph>
class ComponentRemover {
 public:
    typedef typename Graph::EdgeId EdgeId;
    typedef typename Graph::VertexId VertexId;
    typedef std::function<void(const set<EdgeId>&)> HandlerF;

 private:
    Graph& g_;
    HandlerF removal_handler_;

    template<class ElemType>
    void InsertIfNotConjugate(set<ElemType>& elems, ElemType elem) {
        if (elems.count(g_.conjugate(elem)) == 0) {
            elems.insert(elem);
        }
    }

 public:
    ComponentRemover(Graph& g, HandlerF removal_handler = 0)
            : g_(g),
              removal_handler_(removal_handler) {
    }

    template<class EdgeIt>
    void DeleteComponent(EdgeIt begin, EdgeIt end, bool alter_vertices = true) {
        set<EdgeId> edges;
        set<VertexId> vertices;

        //cleaning conjugates and gathering vertices
        for (EdgeIt it = begin; it != end; ++it) {
            EdgeId e = *it;
            InsertIfNotConjugate(edges, e);
            InsertIfNotConjugate(vertices, g_.EdgeStart(e));
            InsertIfNotConjugate(vertices, g_.EdgeEnd(e));
        }

        if (removal_handler_) {
            removal_handler_(edges);
        }

        for (EdgeId e: edges) {
            g_.DeleteEdge(e);
        }

        if (alter_vertices) {
            for (VertexId v: vertices) {
                RemoveIsolatedOrCompress(g_, v);
            }
        }
    }

    template<class Container>
    void DeleteComponent(const Container& container, bool alter_vertices = true) {
        DeleteComponent(container.begin(), container.end(), alter_vertices);
    }

};

}
