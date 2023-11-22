# prepare

Install dependencies

`sudo apt-get install python3-flask python3-gunicorn nginx`

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
