#include "broker.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: broker_app <config.yaml> [producer_endpoint] [consumer_endpoint]\n";
        return 1;
    }
    broker::BrokerApp::Options opts;
    opts.config_path = argv[1];
    if (argc >= 3) opts.producer_endpoint = argv[2];
    if (argc >= 4) opts.consumer_endpoint = argv[3];

    try {
        // 创建BrokerApp实例
        broker::BrokerApp app(opts);
        std::cout << "Broker started. Producers: " << opts.producer_endpoint
                  << " Consumers: " << opts.consumer_endpoint << "\n";
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

