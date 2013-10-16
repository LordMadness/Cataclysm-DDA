#include "dependency_tree.h"

#include <set>
#include "debug.h"

// dependency_node
dependency_node::dependency_node():index(-1), lowlink(-1), on_stack(false)
{
    key = "";
    availability = true;
}

dependency_node::dependency_node(std::string _key):index(-1), lowlink(-1), on_stack(false)
{
    key = _key;
    availability = true;
}

dependency_node::~dependency_node()
{
    parents.clear();
    children.clear();
}

void dependency_node::add_parent(dependency_node* parent)
{
    if (parent)
    {
        parents.push_back(parent);
    }
}

void dependency_node::add_child(dependency_node* child)
{
    if (child)
    {
        children.push_back(child);
    }
}

bool dependency_node::is_available()
{
    return all_errors.size() == 0;
}

std::vector<std::string > dependency_node::errors()
{
    return all_errors;
}
std::string dependency_node::s_errors()
{
    std::string ret = "";
    for (int i = 0; i < all_errors.size(); ++i)
    {
        ret += all_errors[i];
        if (i < all_errors.size() - 1)
        {
            ret += ", ";
        }
    }
    return ret;
}

void dependency_node::check_cyclicity()
{
    std::stack<dependency_node*> nodes_to_check;
    std::set<std::string> nodes_visited;

    DebugLog() << "check_cyclicity: Checking for dependency circularity on ["<<key<<"]\n";

    for (int i = 0; i < parents.size(); ++i)
    {
        nodes_to_check.push(parents[i]);
    }
    nodes_visited.insert(key);

    while (nodes_to_check.size() > 0)
    {
        dependency_node *check = nodes_to_check.top();
        nodes_to_check.pop();

        DebugLog() << "\tchecking ["<<check->key<<"]\n";

        if (nodes_visited.find(check->key) != nodes_visited.end())
        {
            DebugLog() << key<<"::inherit_errors: "<<"Circular Dependency Circuit Found!\n";
            if (std::find(all_errors.begin(), all_errors.end(), "Error: Circular Dependency Circuit Found!") == all_errors.end())
            {
                all_errors.push_back("Error: Circular Dependency Circuit Found!");
            }
            continue;
        }

        // add check parents, if exist, to stack
        if (check->parents.size() > 0)
        {
            for (int i = 0; i < check->parents.size(); ++i)
            {
                nodes_to_check.push(check->parents[i]);
            }
        }
        nodes_visited.insert(check->key);
    }
}

void dependency_node::inherit_errors()
{
    std::stack<dependency_node* > nodes_to_check;
    std::set<std::string> nodes_visited;

    for (int i = 0; i < parents.size(); ++i)
    {
        nodes_to_check.push(parents[i]);
    }
    nodes_visited.insert(key);

    while (nodes_to_check.size() > 0)
    {
        dependency_node *check = nodes_to_check.top();
        nodes_to_check.pop();

        // add check errors
        if (check->errors().size() > 0)
        {
            const std::vector<std::string > cerrors = check->errors();
            for (int i = 0; i < cerrors.size(); ++i)
            {
                if (std::find(all_errors.begin(), all_errors.end(), cerrors[i]) == all_errors.end())
                {
                    all_errors.push_back(cerrors[i]);
                }
            }
        }
        if (nodes_visited.find(check->key) != nodes_visited.end())
        {
            continue;
        }
        // add check parents, if exist, to stack
        if (check->parents.size() > 0)
        {
            for (int i = 0; i < check->parents.size(); ++i)
            {
                nodes_to_check.push(check->parents[i]);
            }
        }
        nodes_visited.insert(check->key);
    }
}

std::vector<std::string > dependency_node::get_dependencies_as_strings()
{
    std::vector<std::string> ret;

    std::vector<dependency_node*> as_nodes = get_dependencies_as_nodes();

    for (int i = 0; i < as_nodes.size(); ++i)
    {
        ret.push_back(as_nodes[i]->key);
    }

    // returns dependencies in a guaranteed valid order
    return ret;
}

std::vector<dependency_node*> dependency_node::get_dependencies_as_nodes()
{
    std::vector<dependency_node*> dependencies;
    std::vector<dependency_node*> ret;
    std::set<std::string> found;

    std::stack<dependency_node*> nodes_to_check;
    for (int i = 0; i < parents.size(); ++i)
    {
        nodes_to_check.push(parents[i]);
    }
    found.insert(key);

    while (nodes_to_check.size() > 0)
    {
        dependency_node *check = nodes_to_check.top();
        nodes_to_check.pop();

        // make sure that the one we are checking is not THIS one
        if (found.find(check->key) != found.end())
        {
            continue; // just keep going, we aren't really caring about availiability right now
        }

        // add check to dependencies
        dependencies.push_back(check);

        // add parents to check list
        if (check->parents.size() > 0)
        {
            for (int i = 0; i < check->parents.size(); ++i)
            {
                nodes_to_check.push(check->parents[i]);
            }
        }
        found.insert(check->key);
    }

    // sort from the back!
    for (int i = dependencies.size()-1; i >= 0; --i)
    {
        if (std::find(ret.begin(), ret.end(), dependencies[i]) == ret.end())
        {
            ret.push_back(dependencies[i]);
        }
    }

    return ret;
}

std::vector<std::string> dependency_node::get_dependents_as_strings()
{
    std::vector<std::string> ret;

    std::vector<dependency_node*> as_nodes = get_dependents_as_nodes();

    for (int i = 0; i < as_nodes.size(); ++i)
    {
        ret.push_back(as_nodes[i]->key);
    }

    // returns dependencies in a guaranteed valid order
    return ret;
}

std::vector<dependency_node*> dependency_node::get_dependents_as_nodes()
{
    std::vector<dependency_node*> dependents, ret;
    std::set<std::string> found;

    std::stack<dependency_node*> nodes_to_check;
    for (int i = 0; i < children.size(); ++i)
    {
        nodes_to_check.push(children[i]);
    }
    found.insert(key);

    while (nodes_to_check.size() > 0)
    {
        dependency_node *check = nodes_to_check.top();
        nodes_to_check.pop();

        if (found.find(check->key) != found.end())
        {
            // skip it because we recursed for some reason
            continue;
        }
        dependents.push_back(check);

        if (check->children.size() > 0)
        {
            for (int i = 0; i < check->children.size(); ++i)
            {
                nodes_to_check.push(check->children[i]);
            }
        }
        found.insert(check->key);
    }

    // sort from front, keeping only one copy of the node
    for (int i = 0; i < dependents.size(); ++i)
    {
        if (std::find(ret.begin(), ret.end(), dependents[i]) == ret.end())
        {
            ret.push_back(dependents[i]);
        }
    }

    return ret;
}



// dependency_tree
dependency_tree::dependency_tree()
{
    //ctor
}

dependency_tree::~dependency_tree()
{
    //dtor
}

void dependency_tree::init(std::map<std::string, std::vector<std::string> > key_dependency_map)
{
DebugLog() << "dependency_tree:: initializing tree\n";
    build_node_map(key_dependency_map);
    build_connections(key_dependency_map);
DebugLog() << "dependency_tree:: tree initialized\n";
}

void dependency_tree::build_node_map(std::map<std::string, std::vector<std::string > > key_dependency_map)
{
    for (std::map<std::string, std::vector<std::string> >::iterator kvp = key_dependency_map.begin(); kvp != key_dependency_map.end(); ++kvp)
    {
        // check to see if the master node map knows the key
        if (master_node_map.find(kvp->first) == master_node_map.end())
        {
            // it does, so get the Node
            master_node_map[kvp->first] = new dependency_node(kvp->first);
        }
    }
}

void dependency_tree::build_connections(std::map<std::string, std::vector<std::string > > key_dependency_map)
{
    for (std::map<std::string, std::vector<std::string> >::iterator kvp = key_dependency_map.begin(); kvp != key_dependency_map.end(); ++kvp)
    {
        // check to see if the master node map knows the key
        if (master_node_map.find(kvp->first) != master_node_map.end())
        {
            // it does, so get the Node
            dependency_node *knode = master_node_map[kvp->first];

            // apply parents list
            std::vector<std::string> vnode_parents = kvp->second;
            for (std::vector<std::string>::iterator vit = vnode_parents.begin(); vit != vnode_parents.end(); ++vit)
            {
                if (master_node_map.find(*vit) != master_node_map.end())
                {
                    dependency_node *vnode = master_node_map[*vit];

                    knode->add_parent(vnode);
                    vnode->add_child(knode);
                }
                else
                {
                    // missing dependency!
                    knode->all_errors.push_back("Missing Dependency: <" + *vit + ">");
                }
            }
        }
    }

    // finalize and check for circular dependencies
    check_for_strongly_connected_components();

    for (std::map<std::string, dependency_node*>::iterator kvp = master_node_map.begin(); kvp != master_node_map.end(); ++kvp)
    {
        DebugLog() << "build_connections: Getting inherited errors from ["<<kvp->first<<"]\n";
        kvp->second->inherit_errors();
    }
}
std::vector<std::string> dependency_tree::get_dependencies_of_X_as_strings(std::string key)
{
    if (master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key]->get_dependencies_as_strings();
    }
    return std::vector<std::string>();
}
std::vector<dependency_node*> dependency_tree::get_dependencies_of_X_as_nodes(std::string key)
{
    if (master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key]->get_dependencies_as_nodes();
    }
    return std::vector<dependency_node*>();
}

std::vector<std::string> dependency_tree::get_dependents_of_X_as_strings(std::string key)
{
    if (master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key]->get_dependents_as_strings();
    }
    return std::vector<std::string>();
}

std::vector<dependency_node*> dependency_tree::get_dependents_of_X_as_nodes(std::string key)
{
    if (master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key]->get_dependents_as_nodes();
    }
    return std::vector<dependency_node*>();
}


bool dependency_tree::is_available(std::string key)
{
    if (master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key]->is_available();
    }

    return false;
}

void dependency_tree::clear()
{
    // remove all keys and nodes from the master_node_map
    if (master_node_map.size() > 0)
    {
        for (std::map<std::string, dependency_node*>::iterator it = master_node_map.begin(); it != master_node_map.end(); ++it)
        {
            delete it->second;
        }

        master_node_map.clear();
    }
}
dependency_node *dependency_tree::get_node(std::string key)
{
    if(master_node_map.find(key) != master_node_map.end())
    {
        return master_node_map[key];
    }
    return NULL;
}

// makes sure to set up Cycle errors properly!
void dependency_tree::check_for_strongly_connected_components()
{
    strongly_connected_components = std::vector<std::vector<dependency_node* > >();
    open_index = 0;
    connection_stack = std::stack<dependency_node*>();

    for (std::map<std::string, dependency_node*>::iterator g = master_node_map.begin(); g != master_node_map.end(); ++g)
    {
        //nodes_on_stack = std::vector<dependency_node*>(); // clear it for the next stack to run
        if (g->second->index < 0)
        {
            DebugLog() << "Starting strong connection for: "<<g->second->key<<"\n";
            strong_connect(g->second);
        }
    }

    // now go through and make a set of these
    std::set<dependency_node*> in_circular_connection;
    DebugLog() << strongly_connected_components.size() << " strongly connected groups\n";
    for (int i = 0; i < strongly_connected_components.size(); ++i)
    {
        DebugLog() << ":["<<i<<"] ";
        if (strongly_connected_components[i].size() > 1)
        {
            DebugLog() << "Circular Connection [" << strongly_connected_components[i].size() << "] members\n";
            for (int j = 0; j < strongly_connected_components[i].size(); ++j)
            {
                DebugLog() << "--"<<strongly_connected_components[i][j]->key << "\n";
                in_circular_connection.insert(strongly_connected_components[i][j]);
            }

        }
        else
        {
            DebugLog() << "SKIPPED for monomember condition\n";
        }
    }
    // now go back through this and give them all the circular error code!
    for (std::set<dependency_node*>::iterator it = in_circular_connection.begin(); it != in_circular_connection.end(); ++it)
    {
        (*it)->all_errors.push_back("ERROR: In Circular Dependency Cycle");
    }
}

void dependency_tree::strong_connect(dependency_node *dnode)
{
    static int times_called = 0;
    static int depth = 0;
    static char depth_char = '-';

    ++depth;
    ++times_called;

    std::string depth_string = std::string(depth, depth_char);
DebugLog() << depth_string << dnode->key<<": Set index/lowlink to ("<<open_index<<")\n";
    dnode->index = open_index;
    dnode->lowlink = open_index;
DebugLog() << depth_string << ": increment index counter\n";
    ++open_index;
DebugLog() << depth_string << ": push to stack\n";
    connection_stack.push(dnode);
    dnode->on_stack = true;

    for (int i = 0; i < dnode->parents.size(); ++i)
    {
        if (dnode->parents[i]->index < 0)
        {
            strong_connect(dnode->parents[i]);
            dnode->lowlink = std::min(dnode->lowlink, dnode->parents[i]->lowlink);
        }
        else if (dnode->parents[i]->on_stack)
        {
            dnode->lowlink = std::min(dnode->lowlink, dnode->parents[i]->index);
        }
    }

    if (dnode->lowlink == dnode->index)
    {
        std::vector<dependency_node*> scc;

        dependency_node *d;
        do
        {
            d = connection_stack.top();
            scc.push_back(d);
            connection_stack.pop();
            d->on_stack = false;
        }while (dnode->key != d->key);

        strongly_connected_components.push_back(scc);
        dnode->on_stack = false;
    }

    --depth;
}
