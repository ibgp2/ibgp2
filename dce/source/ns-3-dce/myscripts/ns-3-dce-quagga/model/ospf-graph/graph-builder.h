/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Marc-Olivier Buob
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Marc-Olivier Buob <marcolivier.buob@orange.fr>
 */

#ifndef OSPF_GRAPH_BUILDER
#define OSPF_GRAPH_BUILDER

#include <ostream>                          // std::ostream
#include <map>                              // std::map
#include <cassert>                          // assert

#include <boost/graph/adjacency_list.hpp>   // boost::vertices, boost::edge
#include <boost/graph/graph_traits.hpp>     // boost::graph_traits
#include <boost/utility.hpp>                // boost::noncopyable

namespace ns3 {

/**
 * \brief Un builder de graphe
 * Tgraph : Le type du graphe
 * Tname: La clé identifiant un sommet
 */

template <class Tgraph, class Tname>
class graph_builder_t :
    boost::noncopyable
{
    public:
        typedef Tgraph graph_t;    /**< Le type du graphe */
        typedef Tname vertex_id_t; /**< Le type d'un vertex bundled */

        typedef typename boost::graph_traits<Tgraph>::vertex_descriptor     vertex_descriptor_t;
        typedef typename boost::graph_traits<Tgraph>::vertex_iterator       vertex_iterator_t;
        typedef typename boost::graph_traits<Tgraph>::edge_descriptor       edge_descriptor_t;
        typedef typename boost::graph_traits<Tgraph>::edge_iterator         edge_iterator_t;
        typedef typename boost::graph_traits<Tgraph>::out_edge_iterator     out_edge_iterator_t;
        typedef typename Tgraph::vertex_bundled                             vertex_bundled_t;
        typedef typename Tgraph::edge_bundled                               edge_bundled_t;

        typedef typename std::map<
            vertex_id_t,
            vertex_descriptor_t
        >  vertex_dictionnary_t;            /**< Le type du dictionnaire name-vertex descriptor */

    protected:
        graph_t & graph;                    /**< Une reference au graphe que l'on construit */
        vertex_dictionnary_t dictionnary;   /**< Le dictionnaire associant un nom et un vertex descriptor */

    public:

        /**
         * \brief Le constructeur
         * \param g La reference au graphe a construire
         * remplir au fil de la construction
         */

        graph_builder_t(graph_t & g):
            graph(g)
        {}

        template <typename Fextract>
        void init_dictionnary(){
            vertex_iterator_t vit,vend;
            for(boost::tie(vit, vend) = boost::vertices(graph); vit != vend; ++vit){
                dictionnary[Fextract()(graph[*vit])] = *vit;
            }
        }

        /**
         * \brief Le constructeur de "copie"
         * \param g La reference au graphe a construire
         * \param d La reference au dictionnaire
         * remplir au fil de la construction
         */

        graph_builder_t(
            graph_t & g,
            const vertex_dictionnary_t & d
         ):
            graph(g),dictionnary(d)
        {}

        /**
         * \brief Accesseur sur le dictionnaire
         * \return une reference sur le dictionnaire nom de sommet -
         * vertex descriptor
         */

        inline const vertex_dictionnary_t & get_vertex_dictionnary() const {
            return dictionnary;
        }

        /**
         * \brief Accesseur sur le graphe construite
         * \return une réference sur le graphe
         */

        inline graph_t & get_graph() const {
            return graph;
        }

        /**
         * \brief Indique si un sommet est deja dans le graphe
         * \param name Le nom du sommet
         * \return true si le vertex appelé "name" est dans le graphe
         */

        inline bool has_vertex(const vertex_id_t & name) const {
            return (dictionnary.find(name) != dictionnary.end());
        }

        /**
         * \brief Recupere le vertex appelé "name"
         * \param name Le nom du sommet
         * \return une paire formee du vertex descriptor et d'un
         * boolean indiquant si le sommet a été trouvé
         */

        inline std::pair<vertex_descriptor_t, bool>
        get_vertex(const vertex_id_t & name) const {
            std::pair<vertex_descriptor_t, bool> ret;
            ret.second = false;
            typename vertex_dictionnary_t::const_iterator it = dictionnary.find(name);
            if (it != dictionnary.end()){
                ret.first = (*it).second;
                ret.second = true;
            }
            return ret;
        }

        /**
         * \brief Ajoute un vertex de nom "name". Le sommet n'est
         * ajouté que si un sommet portant ce nom n'est pas déjà
         * présent dans le graphe.
         * \param name Le nom du sommet
         * \param node Le vertex bundled
         * \return Le vertex descriptor de ce sommet (ajouté ou
         * trouvé)
         */

        inline vertex_descriptor_t add_vertex(
            const vertex_id_t & name,
            const vertex_bundled_t & node
        ) {
            graph_t & g = graph;
            vertex_descriptor_t v;
            bool found;

            boost::tie(v, found) = get_vertex(name);
            if (!found) {
                v = boost::add_vertex(g);
                dictionnary[name] = v;
                g[v] = node;
            }

            return v;
        }

        /**
         * \brief Met à jour le vertex de nom "name"
         * \param name Le nom du sommet
         * \param node Le vertex bundled
         * \return Le vertex descriptor du sommet mis a jour
         */

        inline vertex_descriptor_t update_vertex(
            const vertex_id_t & name,
            const vertex_bundled_t & node
        ) {
            graph_t & g = graph;
            std::pair<vertex_descriptor_t,bool> res = get_vertex(name, false);
            vertex_descriptor_t v = res.first;
            if (!res.second) {
                v = boost::add_vertex(g);
                vertex_id_t & reftmp = g[v];
                reftmp = name;
                dictionnary[name] = v;
            }
            g[v] = node;
            return v;
        }


        /**
         * \brief Supprime les arcs d'un sommet
         * \param vname Le nom du sommet a "clearer"
         * \warning Je ne suis pas sur que cette fonction marche si
         * la structure de graphe utilisée pour les sommets est
         * autre chose qu'un boost::vecS !!
         */

        inline void clear_vertex(const vertex_id_t & vname) {
            graph_t & g = this->graph;
            vertex_descriptor_t v;
            bool vfound;
            boost::tie(v, vfound) = get_vertex(vname);
            if (!vfound) return;
            boost::clear_vertex(v, g);
        }

        /**
         * \brief Supprime un sommet
         * \param vname Le nom du sommet a supprimer
         * \warning Je ne suis pas sur que cette fonction marche si
         * la structure de graphe utilisée pour les sommets est
         * autre chose qu'un boost::vecS !!
         */

        inline void remove_vertex(const vertex_id_t & vname) {
            graph_t & g = this->graph;
            vertex_descriptor_t v;
            bool vfound;
            boost::tie(v,vfound) = get_vertex(vname);
            if (!vfound) return;
            boost::clear_vertex(v,g);
            boost::remove_vertex(v,g);
            dictionnary.erase(vname);
            typename vertex_dictionnary_t::iterator
                dit (dictionnary.begin()),
                dend(dictionnary.end());
            for (; dit != dend; ++dit) {
                if (dit->second > v) --(dit->second);
            }
        }

        /**
         * \brief Ajoute un arc au graphe
         * \param src_name Le nom du sommet source
         * \param dst_name Le nom du sommet destination
         */

        inline std::pair<edge_descriptor_t,bool>
        add_edge(
            const vertex_id_t & src_name,
            const vertex_id_t & dst_name
        ) {
            return add_edge(src_name, dst_name, edge_bundled_t());
        }

        /**
         * \brief Ajoute un arc au graphe
         * \param src_name Le nom du sommet source
         * \param dst_name Le nom du sommet destination
         * \param edge_prop Le edge_bundled à affecter à cet arc
         * \return une paire formée du edge descriptor (ajouté ou trouvé)
         * et d'un booléen indiquant si l'arc a effectivement été ajouté
         */

        inline std::pair<edge_descriptor_t,bool>
        add_edge(
            const vertex_id_t    & src_name,
            const vertex_id_t    & dst_name,
            const edge_bundled_t & edge_prop
        ) {
            std::pair<edge_descriptor_t,bool> ret;
            ret.second = false;
            vertex_descriptor_t src,dst;
            bool found_src,found_dst;
            boost::tie(src,found_src) = get_vertex(src_name);
            assert(found_src);
            boost::tie(dst,found_dst) = get_vertex(dst_name);
            assert(found_dst);
            return add_edge(src,dst,edge_prop);
        }

        /**
         * \brief Ajoute un arc au graphe
         * \param src Le vertex descriptor du sommet source
         * \param dst Le vertex descriptor du sommet destination
         * \return une paire formée du edge descriptor (ajouté ou trouvé)
         * et d'un booléen indiquant si l'arc a effectivement été ajouté
         */

        inline std::pair<edge_descriptor_t,bool>
        add_edge(
            const vertex_descriptor_t & src,
            const vertex_descriptor_t & dst
        ) {
            return add_edge(src,dst,edge_bundled_t());
        }

        /**
         * \brief Ajoute un arc au graphe. Si l'arc était déjà présent il
         * ne se passe rien.
         * \param src Le vertex descriptor du sommet source
         * \param dst Le vertex descriptor du sommet destination
         * \param edge_prop Le edge_bundled à affecter à cet arc
         * \return une paire formée du edge descriptor (ajouté ou trouvé)
         * et d'un booléen indiquant si l'arc a effectivement été ajouté
         */

        inline std::pair<edge_descriptor_t,bool>
        add_edge(
            const vertex_descriptor_t & src,
            const vertex_descriptor_t & dst,
            const edge_bundled_t & edge_prop
        ) {
            graph_t & g =  this->graph;
            std::pair<edge_descriptor_t, bool> ret;
            ret.second = false;
            ret = boost::edge(src,dst,g);
            if (!ret.second) {
                ret = boost::add_edge(src,dst,g);
                bool added = ret.second;
                assert(added);
                if(added)  g[ret.first] = edge_prop;
            }
            return ret;
        }

        /**
         * \brief Met à jour un arc au graphe
         * \param src Le vertex descriptor du sommet source
         * \param dst Le vertex descriptor du sommet destination
         * \param edge_prop Le edge_bundled à affecter à cet arc
         */

        inline std::pair<edge_descriptor_t,bool>
        update_edge(
            const vertex_descriptor_t & src,
            const vertex_descriptor_t & dst,
            const edge_bundled_t & edge_prop
        ) {
            graph_t & g =  this->graph;
            std::pair<edge_descriptor_t, bool> ret;
            ret.second = false;
            ret = boost::edge(src, dst, g);
            if (!ret.second) {
                return add_edge(src,dst,edge_prop);
            }
            g[ret.first] = edge_prop;
            return ret;
        }

        /**
         * \brief Met à jour un arc au graphe
         * \param src_name Le nom du sommet source
         * \param dst_name Le nom du sommet destination
         * \param edge_prop Le edge_bundled à affecter à cet arc
         */

        inline std::pair<edge_descriptor_t,bool>
        update_edge(
            const vertex_id_t & src_name,
            const vertex_id_t & dst_name,
            const edge_bundled_t & edge_prop
        ) {
            vertex_descriptor_t vsrc,vdst;
            bool found_src,found_dst;
            boost::tie(vsrc,found_src) = get_vertex(src_name);
            assert(found_src);
            boost::tie(vdst,found_dst) = get_vertex(dst_name);
            assert(found_dst);
            return update_edge(vsrc,vdst,edge_prop);
        }

        /**
         * \brief Pour récupérer un arc entre deux sommets
         * \param src_name Le nom du sommet source
         * \param dst_name Le nom du sommet destination
         * \return une paire formée du edge_descriptor correspondant et
         * d'un boolean indiquant si l'arc a été trouvé.
         */

        inline std::pair<edge_descriptor_t,bool>
        get_edge(
            const vertex_id_t & src_name,
            const vertex_id_t & dst_name
        ) const {
            const graph_t & g =  this->graph;
            std::pair<edge_descriptor_t,bool> ret;
            ret.second = false;
            bool found_src, found_dst;
            vertex_descriptor_t vsrc, vdst;
            boost::tie(vsrc, found_src) = get_vertex(src_name);
            if (!found_src) return ret;
            boost::tie(vdst, found_dst) = get_vertex(dst_name);
            if (!found_dst) return ret;
            return boost::edge(vsrc,vdst,g);
        }

        /**
         * \brief Supprime un arc
         * \param src_name Le sommet source
         * \param dst_name Le sommet source
         */
        inline void remove_edge(
            const vertex_id_t & src_name,
            const vertex_id_t & dst_name
        ) {
            graph_t & g = this->graph;
            vertex_descriptor_t vsrc, vdst;
            bool vsrc_found, vdst_found;
            boost::tie(vsrc, vsrc_found) = get_vertex(src_name);
            if (!vsrc_found) return;
            boost::tie(vdst,vdst_found) = get_vertex(dst_name);
            if (!vdst_found) return;
            boost::remove_edge(vsrc, vdst, g);
        }

        /**
         * \brief Verifie si un arc est présent dans le graphe
         * \param src_name Le nom du sommet source
         * \param dst_name Le nom du sommet destination
         * \return true si l'arc a été trouvé, false sinon
         */

        bool has_edge(
            const vertex_id_t & src_name,
            const vertex_id_t & dst_name
        ) const {
            const graph_t & g =  this->graph;
            bool found_src, found_dst;
            vertex_descriptor_t vsrc,vdst;
            boost::tie(vsrc, found_src) = get_vertex(src_name);
            if (!found_src) return false;
            boost::tie(vdst, found_dst) = get_vertex(dst_name);
            if (!found_dst) return false;
            return boost::edge(vsrc, vdst, g).second;
        }

        /**
         * \brief Pour récupérer un arc entre deux sommets
         * \param vsrc Le sommet source
         * \param vdst Le sommet destination
         * \return une paire formée du edge_descriptor correspondant et
         * d'un boolean indiquant si l'arc a été trouvé.
         */

        inline std::pair<edge_descriptor_t,bool>
        get_edge(
            const vertex_descriptor_t & vsrc,
            const vertex_descriptor_t & vdst
        ) const {
            graph_t & g =  this->graph;
            return boost::edge(vsrc,vdst,g);
        }
};

/**
 * \brief traits pour le graph_builder.
 * Tgraph : Le type du graphe
 * Tname : La clé identifiant un sommet
 *
 * Exemple d'utilisation :
 *
 *  typedef ns3::graph_builder_traits<Tgraph, Tname> btraits;
 *  btraits::dictionnary d;
 *  btraits::graph d;
 *  btraits::graph_builder buider(g,d);
 */

template <class Tgraph, class Tname>
struct graph_builder_traits {
  typedef graph_builder_t<Tgraph,Tname> graph_builder; /**< Le type du graph builder */
  typedef Tgraph graph; /**< Le type du graphe */
  typedef typename graph_builder_t<Tgraph, Tname>::vertex_dictionnary_t dictionnary; /**< Le type du dictionnaire */
};

} // namespace ns3

#endif
