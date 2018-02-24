# Main
FROM gcc:latest
MAINTAINER Daniel Stotts
RUN apt-get update

# Environment
ENV PROJECT /opt/project
WORKDIR ${PROJECT}

# Install necessary packages
RUN apt-get install -y cmake
RUN apt-get install -y libncurses5-dev
RUN apt-get install -y valgrind

# Install other things


# Add project files
ADD . ${PROJECT}

# Start project
ENTRYPOINT ["/bin/bash"]

EXPOSE 10801