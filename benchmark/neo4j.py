from graphdatascience import GraphDataScience

class Neo4J:
    # ref: https://github.com/tomasonjo/blogs/blob/master/gds_python/gds_python_intro.ipynb
    def __init__(self):
        self.host = "bolt://35.175.152.65:7687" #bolt://localhost:7687
        self.user = "neo4j"
        self.password= "roller-agreement-partition" #update this

        self.gds = GraphDataScience(host, auth=(user, password))

        self.G = None # projected
        print(gds.version())    

    def create_constraint():
        query = """
        CREATE CONSTRAINT FOR (node:Node) REQUIRE node.id IS UNIQUE
        """

        self.gds.run_cypher(query)

    def import(url):
        query = """
        :auto LOAD CSV FROM $url AS row FIELDTERMINATOR '\t' CALL {WITH row MERGE (src:Node {id: toInteger(row[0])}) MERGE (dst:Node {id: toInteger(row[1])}) CREATE (src)-[:Edge]->(dst) } IN TRANSACTIONS OF 1000 ROWS
        """
        params = {'url': url}

        self.gds.run_cypher(query, params)

    def project():
        # query = """
        # CALL gds.graph.project(
        # 'myGraph',
        # 'Node',
        # 'Edge'
        # )
        # """

        # self.gds.run_cypher(query)

        self.G, metadata = gds.graph.project(
            'myGraph',
            'Node',
            'Edge'
            )
        

    def setup(url):
        self.create_constraint()
        self.import(url)
        self.project()

    def drop_graph():
        query = """
        MATCH (n)
        CALL {
        WITH n
        DETACH DELETE n
        } IN TRANSACTIONS
        """

        self.gds.run_cypher(query)

    def drop_projection():
        # query = """
        # CALL gds.graph.drop('myGraph', false) YIELD graphName;
        # """

        # self.gds.run_cypher(query)

        self.G.drop()

    # https://neo4j.com/docs/graph-data-science/current/graph-drop/

    # https://neo4j.com/docs/cypher-manual/current/clauses/call-subquery/#delete-with-call-in-transactions
    def drop():
        # may need to swap the order of delete nodes and drop table.
        self.drop_graph()
        self.drop_projection()
        
# As opposed to the stream mode of algorithms, the stats, mutate, and write modes do not produce a stream of results. Therefore, the results of Python client methods are not Pandas DataFrame. Instead, those methods output the algorithm metadata in Pandas Series format.

    def run_bfs():
# CALL gds.bfs.stats('myGraph', {sourceNode:0})
# YIELD
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
# RETURN
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
    return gds.bfs.stats(self.G, sourceNode = 0) # change sourceNode


    def run_tc():
# CALL gds.triangleCount.stats(
#   'myGraph'
# )
# YIELD
#   globalTriangleCount,
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
# RETURN
#   globalTriangleCount,
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
    return gfs.triangleCount.stats(self.G)

    def run_cc():
# CALL gds.wcc.stats(
#   'myGraph'
# )
# YIELD
#   componentCount,
#   preProcessingMillis,
#   computeMillis,
#   postProcessaingMillis,
#   componentDistribution,
#   configuration
# RETURN
#   componentCount,
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   componentDistribution,
#   configuration

    return gds.wcc.stats(self.G)

    def run_pr():
# CALL gds.pageRank.stats('myGraph', {
#   maxIterations: 20,
#   dampingFactor: 0.85
# })
# YIELD
# ranIterations,
# didConverge,
# preProcessingMillis,
# computeMillis,
# postProcessingMillis,
# centralityDistribution
# RETURN
# ranIterations AS numIterations,
# didConverge AS didConverge,
# preProcessingMillis AS preProcessingTime,
# computeMillis AS computeTime,
# postProcessingMillis AS postProcessingTime,
# centralityDistribution AS centralityDistribution

    return gds.pageRank.stats(self.G, maxIterations = 20, dampingFactor = 0.85)
    # delta stepping alg
    def run_sssp():

# CALL gds.allShortestPaths.delta.stats('myGraph', {sourceNode:0})
# YIELD
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
# RETURN
#   preProcessingMillis,
#   computeMillis,
#   postProcessingMillis,
#   configuration
    return gds.allShortestPaths.delta.stats(self.G, sourceNode = 0)

    def run_bc():
# CALL gds.betweenness.stats('myGraph', {samplingSize:100})
# YIELD
# preProcessingMillis,
# computeMillis,
# postProcessingMillis,
# centralityDistribution
# RETURN
# preProcessingMillis AS preProcessingTime,
# computeMillis AS computeTime,
# postProcessingMillis AS postProcessingTime,
# centralityDistribution AS centralityDistribution
    return gds.betweenness.stats(self.G, samplingSize = 100)

    def run_neo4j_pr(datasets, log): # make these 6 helpers, and run them at the end of each of the 6 functions in run_benchmarks. no need to integrate/couple them with the existing for loops as they have .types (3 database reps, edgelist, adjlist, ...) and .variants (cc, cc_parallel, cc_ec)that we don't need to try...
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" #smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_pr()
            log.write(logs) # write the logs!!! output from neo4j run... PANDAS SERIES
            self.drop()

    def run_neo4j_tc(datasets, log):
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" # smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_tc()
            log.write(logs) # write the logs!!! output from neo4j run...
            self.drop()
    
    def run_neo4j_bfs(datasets, log):
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" # smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_bfs()
            log.write(logs) # write the logs!!! output from neo4j run...
            self.drop()

    def run_neo4j_sssp(datasets, log):
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" # smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_sssp()
            log.write(logs) # write the logs!!! output from neo4j run...
            self.drop()

    def run_neo4j_cc(datasets, log):
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" # smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_cc()
            log.write(logs) # write the logs!!! output from neo4j run...
            self.drop()

    def run_neo4j_bc(datasets, log):
        for dataset in datasets:
            url = "file://drives/hdd_main/{dataset}/graph_{dataset}.csv" # smthing like this
            self.setup(url) # construct url from dataset...
            logs = self.run_bc()
            log.write(logs) # write the logs!!! output from neo4j run...
            self.drop()

    def main():