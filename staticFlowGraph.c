
/*
Based on tutorial given at 
https://www.youtube.com/live/ce496ZCwlqA?t=1121s
*/

/*
This outline is for running a fixed flowgraph (Static application)
*/



/*
We create an instance of the graph which contains/owns 
the blocks we instantiate
*/
gr::Graph graph{};


/*
We emplace blocks within the graph, each block has ports and type
*/

/*
You use the emplaceBlock member function to add  blocks to your graph

you give the emplaceBlock the block you want to add to the graph, its type
and its parameters

//DOUBLE CHECK THIS IS WHATS ACTUALLY BEING RETURNED
You then store the address of this block

*/

//Lets get 3 example blocks ready
//A Source block (CountingSource)
//A Processing block
//A Sink block (CoutingSink)


//Source block
auto& source = graph.emplaceBlock<CountingSource<float>>
({

//This is the maximum number of sampling the counting source will generate
{ "n_samples_max" , 100U }
		
});


//Processing block
auto& processing = graph.emplaceBlock<builtin_multiply<float>>
({

//Factor by which we are multiplying
{ "factor" , 3.0f }
		
});


//Sink block
auto& sink = graph.emplaceBlock<CountingSink<float>>();


/*
Now that we have 3 blocks in our graph we need to connect them together

we use the connect member function where we designate what output port we want 
from a given block to be connect "to" what in port from another block

These connections can be checked to see if they succeded or not
*/

//From source block to processing
auto source_to_processing = graph.connect<"out">(source).to<"in">(processing);
//From processing to sink 
auto processing_to_sink = graph.connect<"out">(scale).to<"in">(sink);

if(source_to_processing == FAILED || processing_to_sink == FAILED)
{
	fmt::print("One of the connections has failed")
}

//Pass the graph to the scheduler
gr::scheduler::simple<> scheduler{std::move(graph)};

scheduler.runAndWait();
