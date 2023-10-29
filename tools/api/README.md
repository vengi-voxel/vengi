# prepare

Install flask

`sudo apt-get install python3-flask`

# run

`./vengi.py`

# test

```sh
for i in $(seq 1 100); do
	curl \
		-H "User-Agent: fake/1.0" \
		-H "Content-Type: application/json" \
		-d "{\"name\": \"start\", \"value\": \"1\", \"tags\": {\"uuid\": \"$(uuidgen)\", \"tagname\": \"tagvalue\"}}' \
		http://127.0.0.1:5000/metric &
done
```

Install grafana with the [sqlite plugin](https://github.com/fr-ser/grafana-sqlite-datasource)

```sh
grafana-cli --pluginUrl https://github.com/fr-ser/grafana-sqlite-datasource/releases/download/v3.3.3/frser-sqlite-datasource-3.3.3.zip plugins install frser-sqlite-datasource
```
