function jsonToTable(data, category, id) {
	// Create the table
	const table = document.createElement('table');
	const header = table.createTHead();
	const row = header.insertRow(0);

	// Create table headers
	const headers = Object.keys(data[0]);
	headers.forEach(headerText => {
		const th = document.createElement('th');
		th.textContent = headerText;
		row.appendChild(th);
	});

	// Create table rows
	const body = table.createTBody();
	table.setAttribute('border', '1');
	data.forEach(item => {
		const row = body.insertRow();
		headers.forEach(header => {
			const cell = row.insertCell();
			cell.textContent = item[header];
		});
	});

	const categoryHeader = document.createElement('h2');
	categoryHeader.textContent = category;

	// Create an h1 element for the category
	const div = document.getElementById(id);
	if (div) {
		div.appendChild(categoryHeader);
		div.appendChild(table);
	} else {
		console.error('Element with ID ' + id + ' not found.');
	}
}
