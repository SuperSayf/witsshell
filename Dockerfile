# Use a Linux base image
FROM ubuntu:latest

# Set the working directory to /app
WORKDIR /app

# Copy everything from your root directory to the container's /app directory
COPY . /app

# Install development essentials, including make
RUN apt-get update && apt-get install -y build-essential make

# Change permissions recursively for all scripts in the src directory
RUN find /app/src -type f -iname "*.sh" -exec chmod +x {} \;

# Change permission for the test.sh script\
RUN chmod +x test.sh

# Run the Bash script
CMD ["./test.sh"]
