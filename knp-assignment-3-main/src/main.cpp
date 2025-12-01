/*
 * KNP module 2 Sample starter code using Restinio library (HTTP with C++) for Client-Server
 * and you can also use this in your PRJ3 project
 * Underviser: "Jenny" Jung Min Kim
 *
 * Purpose:
 *   Minimal example showing how to use the Restinio library to handle HTTP protocol and
 *   HTTP-based Client–Server communication. 
 *   (You’ll learn HTTP and implementations in detail in November in the KNP course.)
 *
 * IMPORTANT — Adapt for your PRJ3 project:
 *   ( You will learn these steps in KNP module 2 gradually)
 *   1) Replace book_t/book_collection/book_collection_t/book_handler_t
 *      with your own data structure, collections, and handlers. 
 *   2) Update the initial values in main() to match your data and logic.
 *   3) Add the HTTP endpoints you need (GET/POST/PUT/DELETE, etc.).
 */

#include <iostream>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>
#include <restinio/websocket/websocket.hpp>

//=========================*/
// Example data structure
// TODO later: Replace 'book_t' with your own struct (e.g. something_t).
//=========================*/
struct place_t{
	place_t() = default;

	place_t(std::string placeName, double lat, double lon) : 
	m_placeName(std::move(placeName)), m_lat(std::move(lat)), m_lon(std::move(lon))
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("placeName", m_placeName)
			& json_dto::mandatory("lat", m_lat)
			& json_dto::mandatory("lon", m_lon);

	}

	std::string m_placeName;
	double m_lat;
	double m_lon;

};


struct weather_t  // This is Data structure (sample code has book_t, you need to define your own data struct)
{
	weather_t() = default;

	weather_t(std::string id, std::string date, std::string time, place_t place, double temperature, double humidity) : 
	m_id(id), m_date(date), m_time(time), m_place(place), m_temperature(temperature), m_humidity(humidity)
	{}

	template < typename JSON_IO >
	void json_io(JSON_IO & io)
	{
		io
			& json_dto::mandatory("id", m_id)
			& json_dto::mandatory("date", m_date)
			& json_dto::mandatory("time", m_time)
			& json_dto::mandatory("place", m_place)
			& json_dto::mandatory("temperature", m_temperature)
			& json_dto::mandatory("humidity", m_humidity);
	}

	std::string m_id;
	std::string m_date;
	std::string m_time;
	place_t m_place;
	double m_temperature;
	double m_humidity;
};

//=========================*/
// Todo later: Change this alias for your project type, e.g. vector<something_t>
//=========================*/
using weather_collection_t = std::vector< weather_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

//websocket
namespace rws = restinio::websocket::basic;
using traits_t =
	restinio::traits_t<
		restinio::asio_timer_manager_t,
		restinio::single_threaded_ostream_logger_t,
		router_t>;
using ws_registry_t = std::map< std::uint64_t, rws::ws_handle_t >; // Alias for container with stored websocket handles.

//=========================*/
// HTTP handler class
// TODO later: Rename/modify methods for your data model
//=========================*/
class weather_handler_t
{
public:
	explicit weather_handler_t(weather_collection_t & weathers)
		: m_weathers(weathers)
	{}

	auto on_weather_list(const restinio::request_handle_t& req, rr::route_params_t) const
	{
		auto resp = init_resp(req->create_response());

		resp.set_body(json_dto::to_json(m_weathers));

		return resp.done();
	}

	auto on_new_weather(const restinio::request_handle_t& req, rr::route_params_t) const 
	{
		auto resp = init_resp(req->create_response());

		try
		{
			m_weathers.emplace_back(json_dto::from_json < weather_t >(req->body()));

			//websocket
			sendMessage("POST: id = " + json_dto::from_json<weather_t>(req->body()).m_id);
		}
		catch(const std::exception &)
		{
			mark_as_bad_request(resp);
		}
		return resp.done();
	}

	auto on_weather_get_id(const restinio::request_handle_t& req, rr::route_params_t params) const
	{
		auto resp = init_resp(req->create_response());
		try
		{
			std::vector<weather_t> weather_date;
			auto id = restinio::utils::unescape_percent_encoding( params[ "id" ] );

			for( std::size_t i = 0; i < m_weathers.size(); ++i ) {
				const auto & b = m_weathers[ i ]; 
				if( id == b.m_id ) {
					weather_date.push_back(b);
				} 
			}
			resp.set_body(json_dto::to_json(weather_date));
		}
		catch(const std::exception &)
		{
			mark_as_bad_request(resp);
		}
		return resp.done();
	}

	auto on_weather_get_date(const restinio::request_handle_t& req, rr::route_params_t params) const
	{ 
		auto resp = init_resp(req->create_response());
		try
		{
			std::vector<weather_t> weather_date;
			auto date = restinio::utils::unescape_percent_encoding( params[ "date" ] );

			for( std::size_t i = 0; i < m_weathers.size(); ++i ) {
				const auto & b = m_weathers[ i ]; 
				if( date == b.m_date ) {
					weather_date.push_back(b);
				} 
			}
			resp.set_body(json_dto::to_json(weather_date));
		}
		catch(const std::exception &)
		{
			mark_as_bad_request(resp);
		}
		return resp.done();
	}

	auto on_weather_get_three(const restinio::request_handle_t& req, rr::route_params_t) const 
	{
		auto resp = init_resp(req->create_response());
		auto count = m_weathers.size();
		auto start = (count > 3 ? count - 3 : 0);
		
		try
		{
			std::vector<weather_t> weather_three;
			for(std::size_t i = count; i > start; --i) {
    			weather_three.push_back(m_weathers[i-1]);
			}
			resp.set_body(json_dto::to_json(weather_three));
		}
		catch(const std::exception &)
		{
			mark_as_bad_request(resp);
		}
		return resp.done();
	}

auto on_weather_update(const restinio::request_handle_t& req, rr::route_params_t params )
{
    auto resp = init_resp( req->create_response() );

    try
    {
        // Hent ID som string
        auto weatherid = restinio::utils::unescape_percent_encoding(params["id"]);
        
        auto b = json_dto::from_json<weather_t>( req->body() );

        // Find element der HAR dette ID (ikke array position!)
        auto item = std::find_if(m_weathers.begin(), m_weathers.end(), 
            [&](const auto& w) {
                return w.m_id == weatherid;  // Sammenligner ID-værdier
            }
        );

        if (item != m_weathers.end()) 
        {
            *item = b;  // Opdaterer det fundne element
            sendMessage("PUT: id = " + weatherid);
            resp.set_body(json_dto::to_json(m_weathers));
        }
        else
        {
            mark_as_bad_request( resp );
            resp.set_body("No weather with id: " + weatherid + "\n");
        }
    }
    catch( const std::exception& )
    {
        mark_as_bad_request( resp );
    }

    return resp.done();
}

	auto on_weather_delete(const restinio::request_handle_t& req, rr::route_params_t params )
	{
		auto resp = init_resp( req->create_response() );

		try {
			// Weather-ID (string eller tal, afhængigt af dit setup)
			auto weatherid =
				restinio::utils::unescape_percent_encoding(
					params["id"]
				);

			// Find element efter ID
			auto item = std::find_if(m_weathers.begin(), m_weathers.end(), [&](const auto& w)
			{
				return w.m_id == weatherid;
			});

			if (item != m_weathers.end()) {
				// Slet fra listen
				m_weathers.erase(item);

				// WebSocket-besked
				sendMessage("DELETE: id = " + weatherid);

				// Returnér opdateret liste som JSON
				resp.set_body(
					json_dto::to_json(m_weathers)
				);
			}
			else {
				// Hvis ID ikke findes
				mark_as_bad_request(resp);
			}
		}
		catch (const std::exception&) {
			mark_as_bad_request(resp);
		}

		return resp.done();
	}

	auto on_live_update(const restinio::request_handle_t& req, rr::route_params_t params )
	{
		// Tjek om klienten vil opgradere til WebSocket
		if (restinio::http_connection_header_t::upgrade ==
			req->header().connection())
		{
			auto wsh = rws::upgrade<traits_t>(
				*req, rws::activation_t::immediate,
				// lambda: WebSocket event handler
				[ &registry = m_registry ](auto wsh, auto m)
				{
					// Tekst / binær / continuation frame
					if (rws::opcode_t::text_frame == m->opcode() ||
						rws::opcode_t::binary_frame == m->opcode() ||
						rws::opcode_t::continuation_frame == m->opcode())
					{
						wsh->send_message(*m);
					}
					// Ping → send pong tilbage
					else if (rws::opcode_t::ping_frame == m->opcode())
					{
						auto resp = *m;
						resp.set_opcode(rws::opcode_t::pong_frame);
						wsh->send_message(resp);
					}
					// Connection close → ryd op
					else if (rws::opcode_t::connection_close_frame ==
							m->opcode())
					{
						registry.erase(wsh->connection_id());
					}
				});

			// Tilføj websocket-forbindelsen i registry
			m_registry.emplace(wsh->connection_id(), wsh);

			// Sig OK til WebSocket-opgradering
			return restinio::request_accepted();
		}

		// Hvis ikke en WebSocket-opgradering
		return restinio::request_rejected();
	}



	// OPTIONS for CORS
	auto options(restinio::request_handle_t req, rr::route_params_t)
	{
		const auto methods = "OPTIONS, GET, POST, PATCH, PUT, DELETE";
		auto resp = init_resp(req->create_response());
		resp.append_header("Access-Control-Allow-Methods", methods);
		resp.append_header("Access-Control-Allow-Headers", "content-type");
		resp.append_header("Access-Control-Max-Age", "86400");
		return resp.done();
	}

private:
	weather_collection_t & m_weathers;  // TODO: Replace 'book_collection_t', m_books with your own ..

	ws_registry_t m_registry;
    
	void sendMessage(std::string message) const {
		for(auto [k, v] : m_registry){
			v->send_message(rws::final_frame, rws::opcode_t::text_frame, message);
		}
	}

	template < typename RESP >
	static RESP init_resp(RESP resp)
	{
		resp
			.append_header("Server", "RESTinio sample server /v.0.6")
			.append_header_date_field()
			.append_header("Content-Type", "application/json") //Todo: Change to "application/json"
			.append_header(restinio::http_field::access_control_allow_origin, "*");  //Todo: add this line to enable CORS - all origin allowed
		return resp;
	}

	static void mark_as_bad_request(auto & resp)  // (auto & resp) with C++20 , if C++17, it should be (RESP & resp)
	{
		resp.header().status_line(restinio::status_bad_request());
	}

};

//=========================*/
// Router setup
// TODO later: Add new endpoints for your project (HTTP POST, PUT, etc.) - we will learn gradually in lecture KNP module2
//=========================*/
auto server_handler(weather_collection_t & weather_collection) //replace book_ .. 
{
	auto router = std::make_unique<router_t>();
	auto handler = std::make_shared<weather_handler_t>(std::ref(weather_collection));

	auto by = [&](auto method) {
		using namespace std::placeholders;
		return std::bind(method, handler, _1, _2);
	};

	// Example: GET /
	router->http_get("/", by(&weather_handler_t::on_weather_list));  //replace book_ .. 
	router->http_post("/", by(&weather_handler_t::on_new_weather));
	router->add_handler(restinio::http_method_options(), "/", by(&weather_handler_t::options));
	
	router->http_get("/date/:date", by(&weather_handler_t::on_weather_get_date));
	router->http_get("/three", by(&weather_handler_t::on_weather_get_three));

	router->http_get(R"(/id/:id(\d+))", by(&weather_handler_t::on_weather_get_id));
	router->add_handler(restinio::http_method_options(), R"(/id/:id(\d+))", by(&weather_handler_t::options));

	router->http_get("/chat", by(&weather_handler_t::on_live_update));
	router->add_handler(restinio::http_method_options(), "/", by(&weather_handler_t::options));

	router->http_delete(R"(/id/:id(\d+))", by(&weather_handler_t::on_weather_delete));
	router->add_handler(restinio::http_method_options(), R"(/id/:id(\d+))", by(&weather_handler_t::options));

	router->http_put(R"(/id/:id(\d+))", by(&weather_handler_t::on_weather_update));
	router->add_handler(restinio::http_method_options(), R"(/id/:id(\d+))", by(&weather_handler_t::options));




	

	return router;
}

//=========================*/
// Main entry point
//=========================*/
int main()
{
	using namespace std::chrono;

	try
	{
		using traits_t = restinio::traits_t<
			restinio::asio_timer_manager_t,
			restinio::single_threaded_ostream_logger_t,
			router_t >;

		//=========================*/
		// TODO later: Replace this hardcoded collection with your own initial values
		//=========================*/
		weather_collection_t weather_collection{
			{"1", "20-03-2025", "13:13", {"Skanderborg", 13.1462, 80.1322}, 20, 5},
			{"2", "18-11-2025", "19:13", {"Tilst", 19.1462, 42.1213412}, 12, 2},
			{"3", "18-11-2025", "19:13", {"Tilst", 19.1462, 42.1213412}, 12, 2},
			{"4", "18-11-2025", "19:13", {"Tilst", 19.1462, 42.1213412}, 12, 2},
			{"5", "18-11-2025", "19:13", {"Tilst", 19.1462, 42.1213412}, 12, 2}
		};

		//=========================*/
		// Run server
		// IMPORTANT:
		//   - "0.0.0.0" means server listens on ALL network interfaces.
		//   - Use Pi's IP + port 8080 to connect from another device.
		//=========================*/
		restinio::run(
			restinio::on_this_thread<traits_t>()
				.address("0.0.0.0")   // For Pi: allow access from outside
				.port(8080)           // Default port, change if needed
				.request_handler(server_handler(weather_collection))
				.read_next_http_message_timelimit(10s)
				.write_http_response_timelimit(1s)
				.handle_request_timeout(1s));
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


